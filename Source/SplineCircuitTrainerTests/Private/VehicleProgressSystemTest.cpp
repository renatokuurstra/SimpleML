//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "Systems/VehicleProgressSystem.h"
#include "Components/TrainingDataComponent.h"
#include "VehicleComponent.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Components/SplineComponent.h"
#include "VehicleLibrary.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

// Helper actor with a spline component
class ASplineTestActor : public AActor
{
public:
	ASplineTestActor()
	{
		SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
		SetRootComponent(SplineComponent);
	}

	UPROPERTY()
	USplineComponent* SplineComponent;
};

TEST_CLASS(SplineCircuitTrainer_VehicleProgressSystem_Tests, "SplineCircuitTrainer.VehicleProgressSystem")
{
	TObjectPtr<UWorld> World;
	TObjectPtr<AVehicleTrainerContext> Context;
	TObjectPtr<ASplineTestActor> SplineActor;
	TObjectPtr<UVehicleProgressSystem> System;
	TObjectPtr<UVehicleTrainerConfig> Config;

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("ProgressTestWorld")));
		ASSERT_THAT(IsNotNull(World, "Transient world should be successfully created"));

		Context = World->SpawnActor<AVehicleTrainerContext>();
		ASSERT_THAT(IsNotNull(Context, "VehicleTrainerContext should be successfully spawned"));

		Config = NewObject<UVehicleTrainerConfig>();
		Context->TrainerConfig = Config;
		Config->MinimumProgressBetweenEvaluations = 10.0f;

		SplineActor = World->SpawnActor<ASplineTestActor>();
		ASSERT_THAT(IsNotNull(SplineActor, "SplineActor should be successfully spawned"));
		
		Context->CircuitActor = SplineActor;

		System = NewObject<UVehicleProgressSystem>();
		ASSERT_THAT(IsNotNull(System, "VehicleProgressSystem should be successfully created"));
		System->Initialize(Context);

		// Setup a simple straight spline from (0,0,0) to (1000,0,0)
		SplineActor->SplineComponent->ClearSplinePoints();
		SplineActor->SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(1000, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->SetClosedLoop(false);
		SplineActor->SplineComponent->UpdateSpline();
	}

	AFTER_EACH()
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
	}

	TEST_METHOD(TracksForwardMovement)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		Pawn->SetActorLocation(FVector(0, 0, 0));

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		Registry.emplace<FVehicleComponent>(Entity, Pawn);
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		UVehicleLibrary::SetTrainingData(Data, SplineActor->SplineComponent, Pawn->GetActorLocation(), World->GetTimeSeconds());

		// First update
		System->Update(0.1f);
		ASSERT_THAT(IsNear(0.0f, Data.DistanceTraveled, 0.1f, "Initially distance traveled should be 0"));

		// Move forward
		Pawn->SetActorLocation(FVector(100, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(100.0f, Data.DistanceTraveled, 1.0f, "Should have traveled 100 units forward"));

		// Move forward again
		Pawn->SetActorLocation(FVector(300, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(300.0f, Data.DistanceTraveled, 1.0f, "Should have traveled 300 units total"));
	}

	TEST_METHOD(TracksBackwardMovement)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		Pawn->SetActorLocation(FVector(500, 0, 0));

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		Registry.emplace<FVehicleComponent>(Entity, Pawn);
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		UVehicleLibrary::SetTrainingData(Data, SplineActor->SplineComponent, Pawn->GetActorLocation(), World->GetTimeSeconds());

		// First update
		System->Update(0.1f);

		// Move backward
		Pawn->SetActorLocation(FVector(400, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(-100.0f, Data.DistanceTraveled, 1.0f, "Should have traveled -100 units backward"));
	}

	TEST_METHOD(HandlesLoopedSpline)
	{
		// Make it a loop with enough segments for the new segment tracking (which uses index < 2 or index > NumSegments - 3)
		// We need at least 5 points for 5 segments to have non-overlapping regions for < 2 and > 2
		SplineActor->SplineComponent->ClearSplinePoints();
		SplineActor->SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(200, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(400, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(600, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(800, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(1000, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->SetClosedLoop(true);
		SplineActor->SplineComponent->UpdateSpline();
		
		float SplineLength = SplineActor->SplineComponent->GetSplineLength();

		APawn* Pawn = World->SpawnActor<APawn>();
		// Start near the end (segment 5)
		Pawn->SetActorLocation(FVector(990, 0, 0));

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		Registry.emplace<FVehicleComponent>(Entity, Pawn);
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		UVehicleLibrary::SetTrainingData(Data, SplineActor->SplineComponent, Pawn->GetActorLocation(), World->GetTimeSeconds());

		System->Update(0.1f);
		
		// Move forward past the wrap point (0) to segment 0
		Pawn->SetActorLocation(FVector(10, 0, 0));
		System->Update(0.1f);
		
		// Expected Delta: (CurrentSplineDist - LastSplineDist) + SplineLength
		// CurrentSplineDist is near 10, LastSplineDist is near 990 (or whatever it is in the loop).
		ASSERT_THAT(IsNear(20.0f, Data.DistanceTraveled, 10.0f, "Should have traveled forward across the loop wrap point"));

		// Move backward past the wrap point back to segment 5
		Pawn->SetActorLocation(FVector(990, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(0.0f, Data.DistanceTraveled, 10.0f, "Should have traveled backward across the loop wrap point back to near 0 total distance"));
	}

	TEST_METHOD(HandlesTeleportDiscontinuity)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		Pawn->SetActorLocation(FVector(0, 0, 0));

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		Registry.emplace<FVehicleComponent>(Entity, Pawn);
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		UVehicleLibrary::SetTrainingData(Data, SplineActor->SplineComponent, Pawn->GetActorLocation(), World->GetTimeSeconds());

		// First update
		System->Update(0.1f);

		// Normal movement
		Pawn->SetActorLocation(FVector(100, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(100.0f, Data.DistanceTraveled, 1.0f, "Should have traveled 100"));

		// Teleport: jump 500 units on a 1000 unit spline (threshold is 20% = 200 units)
		Pawn->SetActorLocation(FVector(600, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(100.0f, Data.DistanceTraveled, 1.0f, "Distance should NOT increase after a teleport jump"));
		ASSERT_THAT(IsNear(600.0f, Data.LastSplineDistance, 1.0f, "LastSplineDistance should still be updated to new location"));

		// Subsequent normal movement should work from new location
		Pawn->SetActorLocation(FVector(700, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(200.0f, Data.DistanceTraveled, 1.0f, "Should have traveled 100 more units after teleport"));
	}

	TEST_METHOD(TracksLastDistanceDelta)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		Pawn->SetActorLocation(FVector(0, 0, 0));

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		Registry.emplace<FVehicleComponent>(Entity, Pawn);
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		UVehicleLibrary::SetTrainingData(Data, SplineActor->SplineComponent, Pawn->GetActorLocation(), World->GetTimeSeconds());

		// Initial update
		System->Update(0.1f);
		ASSERT_THAT(IsNear(0.0f, Data.LastDistanceDelta, 0.1f, "Initially delta should be 0"));

		// Move forward
		Pawn->SetActorLocation(FVector(150, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(150.0f, Data.LastDistanceDelta, 1.0f, "Should have delta of 150"));
	}

	TEST_METHOD(UpdatesTimeSinceLastProgress)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		Pawn->SetActorLocation(FVector(0, 0, 0));

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		Registry.emplace<FVehicleComponent>(Entity, Pawn);
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		UVehicleLibrary::SetTrainingData(Data, SplineActor->SplineComponent, Pawn->GetActorLocation(), World->GetTimeSeconds());

		// Initial update
		System->Update(0.1f);
		ASSERT_THAT(IsNear(0.0f, Data.TimeSinceLastProgress, 0.01f, "Initially time since last progress should be 0"));

		// Move forward enough to reset timer (MinProgress is 10)
		Pawn->SetActorLocation(FVector(20, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(0.0f, Data.TimeSinceLastProgress, 0.01f, "Time since last progress should be reset when delta > MinProgress"));

		// Move forward not enough
		Pawn->SetActorLocation(FVector(25, 0, 0)); // Delta is 5
		System->Update(0.1f);
		ASSERT_THAT(IsNear(0.1f, Data.TimeSinceLastProgress, 0.01f, "Time since last progress should increase when delta <= MinProgress"));

		// Move backward
		Pawn->SetActorLocation(FVector(20, 0, 0)); // Delta is -5
		System->Update(0.1f);
		ASSERT_THAT(IsNear(0.2f, Data.TimeSinceLastProgress, 0.01f, "Time since last progress should increase when moving backward"));

		// Move forward enough again
		Pawn->SetActorLocation(FVector(100, 0, 0)); // Delta is 80
		System->Update(0.1f);
		ASSERT_THAT(IsNear(0.0f, Data.TimeSinceLastProgress, 0.01f, "Time since last progress should be reset again when delta > MinProgress"));
	}
};
