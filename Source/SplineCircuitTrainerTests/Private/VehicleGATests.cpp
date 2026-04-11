//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "TestVehiclePawn.h"
#include "Engine/World.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "Components/NetworkComponent.h"

TEST_CLASS(SplineCircuitTrainer_GA_Tests, "SplineCircuitTrainer.GA")
{
	TObjectPtr<UWorld> World;

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("GATestWorld")));
		ASSERT_THAT(IsNotNull(World, "Transient world should be successfully created"));
	}

	AFTER_EACH()
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
	}

	TEST_METHOD(NextGenerationPreservesElitesAndMarksOthersForReset)
	{
		// 1. Setup config
		UVehicleTrainerConfig* Config = NewObject<UVehicleTrainerConfig>();
		Config->VehiclePawnClass = ATestVehiclePawn::StaticClass();
		Config->Population = 10;
		Config->EliteCount = 2;
		Config->BottomResetFraction = 0.0f; // Disable automatic reset for this test to focus on GA selection
		Config->HiddenLayerSizes = { 4 };
		Config->VehicleOutputCount = 2;

		// 2. Spawn Trainer and Initialize
		AVehicleTrainerContext* Trainer = World->SpawnActor<AVehicleTrainerContext>();
		Trainer->TrainerConfig = Config;
		Trainer->DispatchBeginPlay();

		auto& Registry = Trainer->GetRegistry();

		// 3. Assign fitness values manually
		// Entities [0, 9] have fitness [0.0, 9.0]
		TArray<entt::entity> Population;
		auto View = Registry.view<FFitnessComponent>(entt::exclude_t<FEliteTagComponent>{});
		for (auto E : View)
		{
			Population.Add(E);
		}
		ASSERT_THAT(AreEqual(10, Population.Num(), "Should have 10 entities in population"));

		for (int32 i = 0; i < 10; ++i)
		{
			FFitnessComponent& Fit = Registry.get<FFitnessComponent>(Population[i]);
			Fit.Fitness.SetNum(1);
			Fit.Fitness[0] = (float)i; 

			// Ensure genome view is linked (factory should do it, but we force it here for robustness in test)
			if (Registry.all_of<FGenomeFloatViewComponent, FNeuralNetworkFloat>(Population[i]))
			{
				auto& GView = Registry.get<FGenomeFloatViewComponent>(Population[i]);
				auto& Net = Registry.get<FNeuralNetworkFloat>(Population[i]);
				GView.Values = Net.Network.GetDataView();
			}
		}

		// 4. Trigger Evolution
		// We need to mark entities for reset before calling NextGeneration, 
		// otherwise TournamentSelectionSystem won't pick them for breeding.
		for (auto E : Population)
		{
			Registry.emplace<FResetGenomeComponent>(E);
		}
		
		Trainer->NextGeneration();

		// 5. Verify Elites
		// EliteCount = 2, so the 2 best (entities 8 and 9) should be selected as elites.
		// EliteSelection system creates SEPARATE entities with FEliteTagComponent and FFitnessComponent.
		int32 EliteCount = 0;
		auto EliteView = Registry.view<FEliteTagComponent>();
		for (auto E : EliteView)
		{
			EliteCount++;
		}
		ASSERT_THAT(AreEqual(2, EliteCount, "Should have 2 elite entities after evolution"));

		// 6. Verify Reset Tags on Population
		// All non-elite entities should have been processed.
		// GACleanupSystem removes FResetGenomeComponent, so we expect 0 now.
		int32 ResetCount = 0;
		auto ResetView = Registry.view<FResetGenomeComponent>();
		for (auto E : ResetView)
		{
			ResetCount++;
		}
		ASSERT_THAT(AreEqual(0, ResetCount, "FResetGenomeComponent should be removed by GACleanupSystem at the end of evolution"));

		// 7. Verify Fitness Reset on Population
		// BreedFloatGenomesSystem resets fitness to 0.0 for entities it processes.
		for (auto E : Population)
		{
			const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(0.0f, Fit.Fitness[0], 0.0001f), "Each population entity's fitness should be reset to 0.0 after breeding"));
		}
	}
};
