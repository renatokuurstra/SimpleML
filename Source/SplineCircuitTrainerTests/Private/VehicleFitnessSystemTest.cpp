//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "Systems/VehicleFitnessSystem.h"
#include "Components/TrainingDataComponent.h"
#include "Components/GenomeComponents.h"
#include "EcsContext.h"
#include "Engine/World.h"

TEST_CLASS(SplineCircuitTrainer_VehicleFitnessSystem_Tests, "SplineCircuitTrainer.VehicleFitnessSystem")
{
	TObjectPtr<UWorld> World;
	TObjectPtr<AEcsContext> Context;
	TObjectPtr<UVehicleFitnessSystem> System;

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("FitnessTestWorld")));
		ASSERT_THAT(IsNotNull(World, "Transient world should be successfully created"));

		Context = World->SpawnActor<AEcsContext>();
		ASSERT_THAT(IsNotNull(Context, "AEcsContext should be successfully spawned"));

		System = NewObject<UVehicleFitnessSystem>();
		ASSERT_THAT(IsNotNull(System, "VehicleFitnessSystem should be successfully created"));
		System->Initialize(Context);
	}

	AFTER_EACH()
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
	}

	TEST_METHOD(CalculatesCubeOfDistance)
	{
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		
		FTrainingDataComponent& TrainingData = Registry.emplace<FTrainingDataComponent>(Entity);
		FFitnessComponent& FitComp = Registry.emplace<FFitnessComponent>(Entity);
		FitComp.Fitness.SetNum(1);
		FitComp.Fitness[0] = 0.0f;

		// Set distance to 2
		TrainingData.DistanceTraveled = 2.0f;
		
		// Run system update
		System->Update(0.1f);

		// Expected fitness: 0 + (2^3) = 8
		ASSERT_THAT(IsNear(8.0f, FitComp.Fitness[0], 0.01f, "Fitness should be the cube of distance traveled"));

		// Run again, it should add another cube
		System->Update(0.1f);
		// Expected fitness: 8 + 8 = 16
		ASSERT_THAT(IsNear(16.0f, FitComp.Fitness[0], 0.01f, "Fitness should accumulate another cube of distance traveled"));
	}

	TEST_METHOD(HandlesMultipleEntities)
	{
		entt::registry& Registry = Context->GetRegistry();
		
		entt::entity E1 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E1).DistanceTraveled = 3.0f;
		Registry.emplace<FFitnessComponent>(E1).Fitness.Add(0.0f);

		entt::entity E2 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E2).DistanceTraveled = 4.0f;
		Registry.emplace<FFitnessComponent>(E2).Fitness.Add(10.0f); // Initial 10

		System->Update(0.1f);

		// E1: 0 + 3^3 = 27
		ASSERT_THAT(IsNear(27.0f, Registry.get<FFitnessComponent>(E1).Fitness[0], 0.01f, "E1 fitness should be 27"));
		
		// E2: 10 + 4^3 = 10 + 64 = 74
		ASSERT_THAT(IsNear(74.0f, Registry.get<FFitnessComponent>(E2).Fitness[0], 0.01f, "E2 fitness should be 74"));
	}

	TEST_METHOD(MarksNegativeFitnessForReset)
	{
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		
		Registry.emplace<FTrainingDataComponent>(Entity).DistanceTraveled = 0.0f;
		FFitnessComponent& FitComp = Registry.emplace<FFitnessComponent>(Entity);
		FitComp.Fitness.SetNum(1);
		FitComp.Fitness[0] = -10.0f; // Start with negative fitness

		// Run system update
		System->Update(0.1f);

		// Verify FResetGenomeComponent is added
		ASSERT_THAT(IsTrue(Registry.all_of<FResetGenomeComponent>(Entity), "Entity with negative fitness should be marked for reset with FResetGenomeComponent"));
	}
};
