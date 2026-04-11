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
		Config->NoProgressTimeout = 1.0f;
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

	TEST_METHOD(FlagsNoProgressVehicle)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		Pawn->SetActorLocation(FVector(100, 0, 0));

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		Registry.emplace<FVehicleComponent>(Entity, Pawn);
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		
		// Initial state
		Data.DistanceTraveled = 50.0f;
		Data.MaxDistanceTraveled = 50.0f;
		Data.TimeSinceLastProgress = 0.0f;

		// Update with no progress
		FlagSystem->Update(0.6f);
		ASSERT_THAT(IsFalse(Registry.all_of<FResetGenomeComponent>(Entity), "Should not be flagged yet (0.6s < 1.0s)"));

		FlagSystem->Update(0.5f);
		ASSERT_THAT(IsTrue(Registry.all_of<FResetGenomeComponent>(Entity), "Should be flagged after 1.1s of no progress"));
	}
};
