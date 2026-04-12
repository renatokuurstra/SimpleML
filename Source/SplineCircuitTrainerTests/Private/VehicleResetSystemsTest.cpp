//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "Systems/VehicleResetFlagSystem.h"
#include "Components/TrainingDataComponent.h"
#include "VehicleComponent.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Components/SplineComponent.h"
#include "Components/GenomeComponents.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

// Reuse SplineTestActor from other tests if possible, but defining it here for independence
class ASplineResetTestActor : public AActor
{
public:
	ASplineResetTestActor()
	{
		SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
		SetRootComponent(SplineComponent);
	}
	UPROPERTY()
	USplineComponent* SplineComponent;
};

TEST_CLASS(SplineCircuitTrainer_VehicleResetSystems_Tests, "SplineCircuitTrainer.VehicleResetSystems")
{
	TObjectPtr<UWorld> World;
	TObjectPtr<AVehicleTrainerContext> Context;
	TObjectPtr<ASplineResetTestActor> SplineActor;
	TObjectPtr<UVehicleResetFlagSystem> FlagSystem;
	TObjectPtr<UVehicleTrainerConfig> Config;

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("ResetTestWorld")));
		Context = World->SpawnActor<AVehicleTrainerContext>();
		SplineActor = World->SpawnActor<ASplineResetTestActor>();
		Context->CircuitActor = SplineActor;

		Config = NewObject<UVehicleTrainerConfig>();
		Config->MaxSplineDistanceThreshold = 1000.0f;
		Config->MinAverageVelocity = 100.0f;
		Config->MinAgeForReset = 1.0f;
		Context->TrainerConfig = Config;

		FlagSystem = NewObject<UVehicleResetFlagSystem>();
		FlagSystem->Initialize(Context);

		SplineActor->SplineComponent->ClearSplinePoints();
		SplineActor->SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(5000, 0, 0), ESplineCoordinateSpace::World);
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

	TEST_METHOD(FlagsOffTrackVehicle)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		// Move far away from (0,0,0) -> (5000,0,0) line
		Pawn->SetActorLocation(FVector(100, 2000, 0));

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		Registry.emplace<FVehicleComponent>(Entity, Pawn);
		Registry.emplace<FTrainingDataComponent>(Entity);

		FlagSystem->Update(0.1f);

		ASSERT_THAT(IsTrue(Registry.all_of<FResetGenomeComponent>(Entity), "Entity should be flagged for reset because it is too far from spline"));
	}

	TEST_METHOD(FlagsInsufficientAverageVelocity)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		Pawn->SetActorLocation(FVector(100, 0, 0));

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		Registry.emplace<FVehicleComponent>(Entity, Pawn);
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		
		// Configured MinAverageVelocity is 100.0 cm/s, MinAgeForReset is 1.0s
		Data.CreationTime = 0.0f; 
		Data.DistanceTraveled = 50.0f;

		// Mock CurrentTime = 0.5s by using a trick if possible, or just assume World->GetTimeSeconds() is 0 for now.
		// Actually, since we can't easily advance World time in a simple unit test without Ticking,
		// and we don't want to Tick.
		// But the system uses TrainerContext->GetWorld()->GetTimeSeconds().
		
		// Let's set CreationTime to -0.5f so Age = 0.5s
		Data.CreationTime = World->GetTimeSeconds() - 0.5f;
		FlagSystem->Update(0.1f);
		ASSERT_THAT(IsFalse(Registry.all_of<FResetGenomeComponent>(Entity), "Should not be flagged yet (Age 0.5s < 1.0s)"));

		// Set CreationTime to -1.5f so Age = 1.5s
		Data.CreationTime = World->GetTimeSeconds() - 1.5f;
		// Distance 50, Age 1.5 -> Velocity 33.3 < 100
		FlagSystem->Update(0.1f);
		ASSERT_THAT(IsTrue(Registry.all_of<FResetGenomeComponent>(Entity), "Should be flagged after 1.5s because avg velocity (33.3) < 100"));
	}
};
