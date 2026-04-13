//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "Systems/VehicleFitnessEligibilitySystem.h"
#include "Components/TrainingDataComponent.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Engine/World.h"

TEST_CLASS(SplineCircuitTrainer_VehicleFitnessEligibilitySystem_Tests, "SplineCircuitTrainer.VehicleFitnessEligibilitySystem")
{
	TObjectPtr<UWorld> World;
	TObjectPtr<AVehicleTrainerContext> Context;
	TObjectPtr<UVehicleTrainerConfig> Config;
	TObjectPtr<UVehicleFitnessEligibilitySystem> System;

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("EligibilityTestWorld")));
		ASSERT_THAT(IsNotNull(World, "Transient world should be successfully created"));

		Context = World->SpawnActor<AVehicleTrainerContext>();
		ASSERT_THAT(IsNotNull(Context, "AVehicleTrainerContext should be successfully spawned"));

		Config = NewObject<UVehicleTrainerConfig>();
		Context->TrainerConfig = Config;
		
		Config->MinBreedAge = 10.0f;
		Config->OldestAliveAgeFactor = 0.5f;

		System = NewObject<UVehicleFitnessEligibilitySystem>();
		ASSERT_THAT(IsNotNull(System, "VehicleFitnessEligibilitySystem should be successfully created"));
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

	TEST_METHOD(AddsFitnessComponentWhenMinAgeMet)
	{
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		
		float StartTime = World->GetTimeSeconds();
		Data.CreationTime = StartTime;

		// Initial check - should not have fitness
		System->Update(0.1f);
		ASSERT_THAT(IsFalse(Registry.all_of<FFitnessComponent>(Entity), "Entity should not have fitness component initially"));

		// "Fast forward" time in the component (since we can't easily advance World time in a simple test without ticking)
		Data.CreationTime = StartTime - 11.0f; // Age = 11 > MinBreedAge 10
		
		System->Update(0.1f);
		ASSERT_THAT(IsTrue(Registry.all_of<FFitnessComponent>(Entity), "Entity should have fitness component after MinBreedAge is met"));
		
		FFitnessComponent& Fit = Registry.get<FFitnessComponent>(Entity);
		ASSERT_THAT(AreEqual(1, Fit.Fitness.Num(), "Fitness array should be initialized"));
		ASSERT_THAT(IsNear(0.0f, Fit.Fitness[0], 0.0001f, "Fitness value should be 0.0"));
	}

	TEST_METHOD(RespectsOldestAliveAgeFactor)
	{
		entt::registry& Registry = Context->GetRegistry();
		float CurrentTime = World->GetTimeSeconds();

		// E1 is the oldest (Age 100)
		entt::entity E1 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E1).CreationTime = CurrentTime - 100.0f;

		// E2 is Age 40 (Less than 0.5 * 100 = 50)
		entt::entity E2 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E2).CreationTime = CurrentTime - 40.0f;

		// E3 is Age 60 (More than 0.5 * 100 = 50)
		entt::entity E3 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E3).CreationTime = CurrentTime - 60.0f;

		System->Update(0.1f);

		ASSERT_THAT(IsTrue(Registry.all_of<FFitnessComponent>(E1), "Oldest entity should have fitness"));
		ASSERT_THAT(IsFalse(Registry.all_of<FFitnessComponent>(E2), "E2 age (40) is less than factor (0.5 * 100 = 50), should NOT have fitness"));
		ASSERT_THAT(IsTrue(Registry.all_of<FFitnessComponent>(E3), "E3 age (60) is more than factor (0.5 * 100 = 50), should have fitness"));
	}

	TEST_METHOD(RequiresResetTag)
	{
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		
		float CurrentTime = World->GetTimeSeconds();
		Data.CreationTime = CurrentTime - 20.0f; // Age 20 > MinBreedAge 10

		// Should not have fitness because it's not flagged for reset
		System->Update(0.1f);
		ASSERT_THAT(IsFalse(Registry.all_of<FFitnessComponent>(Entity), "Entity should not have fitness component if NOT flagged for reset"));

		// Flag for reset
		Registry.emplace<FResetGenomeComponent>(Entity, FResetGenomeComponent{ FName("SomeReason") });
		
		System->Update(0.1f);
		ASSERT_THAT(IsTrue(Registry.all_of<FFitnessComponent>(Entity), "Entity should have fitness component after being flagged for reset"));
	}

	TEST_METHOD(BlocksBreedBasedOnReason)
	{
		entt::registry& Registry = Context->GetRegistry();
		FName BlockedReason = FName("BlockedReason");
		FName AllowedReason = FName("AllowedReason");

		FResetReasonConfig BlockedConfig;
		BlockedConfig.Reason = BlockedReason;
		BlockedConfig.bBlockBreed = true;
		Config->ResetReasonConfigs.Add(BlockedConfig);

		float CurrentTime = World->GetTimeSeconds();

		// E1 has blocked reason
		entt::entity E1 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E1).CreationTime = CurrentTime - 20.0f;
		Registry.emplace<FResetGenomeComponent>(E1, FResetGenomeComponent{ BlockedReason });

		// E2 has allowed reason
		entt::entity E2 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E2).CreationTime = CurrentTime - 20.0f;
		Registry.emplace<FResetGenomeComponent>(E2, FResetGenomeComponent{ AllowedReason });

		System->Update(0.1f);

		ASSERT_THAT(IsFalse(Registry.all_of<FFitnessComponent>(E1), "E1 has blocked reason, should NOT have fitness"));
		ASSERT_THAT(IsTrue(Registry.all_of<FFitnessComponent>(E2), "E2 has allowed reason, should have fitness"));
	}
};
