//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "VehicleTrainerManager.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "Components/NetworkComponent.h"
#include "Components/NNIOComponents.h"

TEST_CLASS(VehicleTrainer_Nuke_Tests, "SplineCircuitTrainer.Nuke")
{
	TObjectPtr<UWorld> World;
	TObjectPtr<AVehicleTrainerManager> Manager;
	TObjectPtr<UVehicleTrainerConfig> Config;

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("NukeTestWorld")));
		
		Config = NewObject<UVehicleTrainerConfig>();
		Config->bEnableNuke = true;
		Config->StalenessThreshold = 0.01f;
		Config->StalenessCooldown = 50.0f;
		Config->MinHistoryForStaleness = 50;
		Config->Population = 5;
		Config->EliteCount = 2;

		Manager = World->SpawnActor<AVehicleTrainerManager>();
		Manager->TrainerConfig = Config;
		Manager->NumContexts = 2;
		
		// We need a dummy circuit actor for SpawnContexts to work
		AActor* DummyCircuit = World->SpawnActor<AActor>();
		Manager->CircuitActor = DummyCircuit;
	}

	AFTER_EACH()
	{
		if (World)
		{
			World->DestroyWorld(true);
			World = nullptr;
		}
	}

	TEST_METHOD(StalenessDetectionTriggersNuke)
	{
		// 1. Spawn contexts
		Manager->SpawnContexts();
		
		TArray<AVehicleTrainerContext*> Contexts;
		for (TActorIterator<AVehicleTrainerContext> It(World); It; ++It)
		{
			Contexts.Add(*It);
		}
		
		ASSERT_THAT(AreEqual(2, Contexts.Num(), "Should have 2 contexts"));

		AVehicleTrainerContext* BestContext = Contexts[0];
		AVehicleTrainerContext* StaleContext = Contexts[1];

		// 2. Setup Best Context with a high-performing elite
		{
			auto& BestRegistry = BestContext->GetRegistry();
			auto BestElite = BestRegistry.create();
			BestRegistry.emplace<FEliteTagComponent>(BestElite);
			
			FFitnessComponent Fit; Fit.Fitness.Add(1000.0f);
			BestRegistry.emplace<FFitnessComponent>(BestElite, Fit);
			
			FNeuralNetworkFloat NN;
			TArray<FNeuralNetworkLayerDescriptor> Layers;
			Layers.Add(FNeuralNetworkLayerDescriptor(1));
			Layers.Add(FNeuralNetworkLayerDescriptor(1));
			
			NN.Network.Initialize(Layers);
			NN.Network.GetData()[0] = 42.0f; // Unique weight to identify this pioneer
			BestRegistry.emplace<FNeuralNetworkFloat>(BestElite, NN);
		}

		// 3. Setup Stale Context with low performance and stagnant history
		{
			auto& StaleRegistry = StaleContext->GetRegistry();
			
			// Add elite
			auto StaleElite = StaleRegistry.create();
			StaleRegistry.emplace<FEliteTagComponent>(StaleElite);
			FFitnessComponent Fit; Fit.Fitness.Add(10.0f);
			StaleRegistry.emplace<FFitnessComponent>(StaleElite, Fit);
			
			FNeuralNetworkFloat NN;
			TArray<FNeuralNetworkLayerDescriptor> Layers;
			Layers.Add(FNeuralNetworkLayerDescriptor(1));
			Layers.Add(FNeuralNetworkLayerDescriptor(1));
			
			NN.Network.Initialize(Layers);
			NN.Network.GetData()[0] = 0.0f;
			StaleRegistry.emplace<FNeuralNetworkFloat>(StaleElite, NN);

			// Add some non-elites
			for(int i=0; i<3; ++i) 
			{
				(void)StaleRegistry.create();
			}

			// Add stagnant history
			auto DebugEntity = StaleRegistry.create();
			FGeneticAlgorithmDebugComponent DebugComp;
			for(int i=0; i<60; ++i) DebugComp.HistoricalTotalEliteFitness.Add(10.0f);
			StaleRegistry.emplace<FGeneticAlgorithmDebugComponent>(DebugEntity, DebugComp);
		}

		// 4. Run Nuke Check
		// We need to simulate World Time for cooldowns
		// Since Tick is protected, we can use World->Tick
		World->Tick(LEVELTICK_All, 0.1f);

		// 5. Verify Nuke occurred in StaleContext
		{
			auto& StaleRegistry = StaleContext->GetRegistry();
			
			// Check pioneer injection
			auto EliteView = StaleRegistry.view<FEliteTagComponent, FNeuralNetworkFloat>();
			bool bPioneerFound = false;
			for(auto E : EliteView)
			{
				const auto& NN = EliteView.get<FNeuralNetworkFloat>(E);
				if (NN.Network.GetData()[0] == 42.0f)
				{
					bPioneerFound = true;
					// Verify fitness was reset
					if (StaleRegistry.any_of<FFitnessComponent>(E))
					{
						ASSERT_THAT(AreEqual(0, StaleRegistry.get<FFitnessComponent>(E).Fitness.Num(), "Pioneer fitness should be reset"));
					}
				}
			}
			ASSERT_THAT(IsTrue(bPioneerFound, "Best elite from BestContext should have been injected into StaleContext"));

			// Check reset components on non-elites
			int32 ResetsFound = 0;
			auto NonEliteView = StaleRegistry.view<entt::entity>(entt::exclude<FEliteTagComponent>);
			for(auto E : NonEliteView)
			{
				if (StaleRegistry.any_of<FResetGenomeComponent>(E))
				{
					ResetsFound++;
				}
			}
			ASSERT_THAT(AreEqual(4, ResetsFound, "All non-elites should have FResetGenomeComponent"));

			// Check cooldown
			ASSERT_THAT(IsTrue(StaleContext->NextNukeAvailableTime > 0.0f, "Cooldown should be set on stale context"));
		}
	}
};
