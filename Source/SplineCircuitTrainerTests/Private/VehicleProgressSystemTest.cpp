//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "Systems/VehicleProgressSystem.h"
#include "Components/TrainingDataComponent.h"
#include "VehicleComponent.h"
#include "VehicleTrainerContext.h"
#include "Components/SplineComponent.h"
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

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("ProgressTestWorld")));
		ASSERT_THAT(IsNotNull(World, "Transient world should be successfully created"));

		Context = World->SpawnActor<AVehicleTrainerContext>();
		ASSERT_THAT(IsNotNull(Context, "VehicleTrainerContext should be successfully spawned"));

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
		Registry.emplace<FTrainingDataComponent>(Entity);

		// First update to set LastSplineDistance
		System->Update(0.1f);
		FTrainingDataComponent& Data = Registry.get<FTrainingDataComponent>(Entity);
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
		Registry.emplace<FTrainingDataComponent>(Entity);

		// First update to set LastSplineDistance
		System->Update(0.1f);
		FTrainingDataComponent& Data = Registry.get<FTrainingDataComponent>(Entity);

		// Move backward
		Pawn->SetActorLocation(FVector(400, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(-100.0f, Data.DistanceTraveled, 1.0f, "Should have traveled -100 units backward"));
	}

	TEST_METHOD(HandlesLoopedSpline)
	{
		// Make it a loop
		SplineActor->SplineComponent->SetClosedLoop(true);
		SplineActor->SplineComponent->UpdateSpline();
		float SplineLength = SplineActor->SplineComponent->GetSplineLength();

		APawn* Pawn = World->SpawnActor<APawn>();
		// Start near the end
		Pawn->SetActorLocation(FVector(990, 0, 0));

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		Registry.emplace<FVehicleComponent>(Entity, Pawn);
		Registry.emplace<FTrainingDataComponent>(Entity);

		System->Update(0.1f);
		FTrainingDataComponent& Data = Registry.get<FTrainingDataComponent>(Entity);
		float StartSplineDist = Data.LastSplineDistance;

		// Move forward past the wrap point (0)
		Pawn->SetActorLocation(FVector(10, 0, 0));
		System->Update(0.1f);
		
		// Expected Delta: (CurrentSplineDist - LastSplineDist) + SplineLength
		// CurrentSplineDist is near 10, LastSplineDist is near 990.
		// Delta is near -980. After wrapping: -980 + 1000 = 20.
		ASSERT_THAT(IsNear(20.0f, Data.DistanceTraveled, 2.0f, "Should have traveled forward across the loop wrap point"));

		// Move backward past the wrap point
		Pawn->SetActorLocation(FVector(990, 0, 0));
		System->Update(0.1f);
		ASSERT_THAT(IsNear(0.0f, Data.DistanceTraveled, 2.0f, "Should have traveled backward across the loop wrap point back to 0"));
	}
};
