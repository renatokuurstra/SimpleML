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
		Config->HighestFitnessFactor = 0.5f;

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

	TEST_METHOD(AddsEligibilityTagWhenMinAgeMet)
	{
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		Registry.emplace<FResetGenomeComponent>(Entity);
		Registry.emplace<FFitnessComponent>(Entity);
		
		float StartTime = World->GetTimeSeconds();
		Data.CreationTime = StartTime;

		// Initial check - should not be eligible
		System->Update(0.1f);
		ASSERT_THAT(IsFalse(Registry.all_of<FEligibleForBreedingTagComponent>(Entity), "Entity should not be eligible initially"));

		// "Fast forward" time in the component
		Data.CreationTime = StartTime - 11.0f; // Age = 11 > MinBreedAge 10
		
		System->Update(0.1f);
		ASSERT_THAT(IsTrue(Registry.all_of<FEligibleForBreedingTagComponent>(Entity), "Entity should be eligible after MinBreedAge is met (and MaxFitness is -MAX_FLT)"));
	}

	TEST_METHOD(RespectsHighestFitnessFactor)
	{
		entt::registry& Registry = Context->GetRegistry();
		float CurrentTime = World->GetTimeSeconds();

		// E1 has high fitness (100)
		entt::entity E1 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E1).CreationTime = CurrentTime - 20.0f;
		Registry.emplace<FResetGenomeComponent>(E1);
		FFitnessComponent& F1 = Registry.emplace<FFitnessComponent>(E1);
		F1.Fitness.Add(100.0f);

		// E2 has low fitness (40) - less than 0.5 * 100 = 50
		entt::entity E2 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E2).CreationTime = CurrentTime - 20.0f;
		Registry.emplace<FResetGenomeComponent>(E2);
		FFitnessComponent& F2 = Registry.emplace<FFitnessComponent>(E2);
		F2.Fitness.Add(40.0f);

		// E3 has medium fitness (60) - more than 0.5 * 100 = 50
		entt::entity E3 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E3).CreationTime = CurrentTime - 20.0f;
		Registry.emplace<FResetGenomeComponent>(E3);
		FFitnessComponent& F3 = Registry.emplace<FFitnessComponent>(E3);
		F3.Fitness.Add(60.0f);

		System->Update(0.1f);

		ASSERT_THAT(IsTrue(Registry.all_of<FEligibleForBreedingTagComponent>(E1), "High fitness entity should be eligible"));
		ASSERT_THAT(IsFalse(Registry.all_of<FEligibleForBreedingTagComponent>(E2), "E2 fitness (40) is less than factor (0.5 * 100 = 50), should NOT be eligible"));
		ASSERT_THAT(IsTrue(Registry.all_of<FEligibleForBreedingTagComponent>(E3), "E3 fitness (60) is more than factor (0.5 * 100 = 50), should be eligible"));
	}

	TEST_METHOD(ElitesAreAlwaysEligible)
	{
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Elite = Registry.create();
		Registry.emplace<FEliteTagComponent>(Elite);
		// Elites don't need FResetGenomeComponent or MinBreedAge for eligibility in the new system

		System->Update(0.1f);
		ASSERT_THAT(IsTrue(Registry.all_of<FEligibleForBreedingTagComponent>(Elite), "Elites should always be eligible"));
	}

	TEST_METHOD(RequiresResetTag)
	{
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();
		FTrainingDataComponent& Data = Registry.emplace<FTrainingDataComponent>(Entity);
		Registry.emplace<FFitnessComponent>(Entity);
		
		float CurrentTime = World->GetTimeSeconds();
		Data.CreationTime = CurrentTime - 20.0f; // Age 20 > MinBreedAge 10

		// Should not be eligible because it's not flagged for reset
		System->Update(0.1f);
		ASSERT_THAT(IsFalse(Registry.all_of<FEligibleForBreedingTagComponent>(Entity), "Entity should not be eligible if NOT flagged for reset"));

		// Flag for reset
		Registry.emplace<FResetGenomeComponent>(Entity, FResetGenomeComponent{ FName("SomeReason") });
		
		System->Update(0.1f);
		ASSERT_THAT(IsTrue(Registry.all_of<FEligibleForBreedingTagComponent>(Entity), "Entity should be eligible after being flagged for reset"));
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
		Registry.emplace<FFitnessComponent>(E1);

		// E2 has allowed reason
		entt::entity E2 = Registry.create();
		Registry.emplace<FTrainingDataComponent>(E2).CreationTime = CurrentTime - 20.0f;
		Registry.emplace<FResetGenomeComponent>(E2, FResetGenomeComponent{ AllowedReason });
		Registry.emplace<FFitnessComponent>(E2);

		System->Update(0.1f);

		ASSERT_THAT(IsFalse(Registry.all_of<FEligibleForBreedingTagComponent>(E1), "E1 has blocked reason, should NOT be eligible"));
		ASSERT_THAT(IsTrue(Registry.all_of<FEligibleForBreedingTagComponent>(E2), "E2 has allowed reason, should be eligible"));
	}
};
