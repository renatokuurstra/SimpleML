//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "VehicleTrainerContext.h"
#include "Engine/World.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "Components/BreedingPairComponent.h"
#include "Systems/GADebugDataSystem.h"

TEST_CLASS(GeneticAlgorithm_Debug_Tests, "GeneticAlgorithm.Debug")
{
	TObjectPtr<UWorld> World;
	TObjectPtr<AVehicleTrainerContext> Context;

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("DebugDataTestWorld")));
		Context = World->SpawnActor<AVehicleTrainerContext>();
		ASSERT_THAT(IsNotNull(Context, "AVehicleTrainerContext should be successfully spawned"));
	}

	AFTER_EACH()
	{
		if (World)
		{
			World->DestroyWorld(true);
			World = nullptr;
		}
	}

	TEST_METHOD(GADebugDataSystemCollectsInformation)
	{
		auto& Registry = Context->GetRegistry();
		
		// Create debug entity
		const entt::entity DebugEntity = Registry.create();
		Registry.emplace<FGeneticAlgorithmDebugComponent>(DebugEntity);

		// 1. Add some reset entities
		Registry.emplace<FResetGenomeComponent>(Registry.create());
		Registry.emplace<FResetGenomeComponent>(Registry.create());

		// 2. Add some elites
		auto Elite1 = Registry.create();
		Registry.emplace<FEliteTagComponent>(Elite1);
		FFitnessComponent Fit1; Fit1.Fitness.Add(100.0f);
		Registry.emplace<FFitnessComponent>(Elite1, Fit1);

		auto Elite2 = Registry.create();
		Registry.emplace<FEliteTagComponent>(Elite2);
		FFitnessComponent Fit2; Fit2.Fitness.Add(200.0f);
		Registry.emplace<FFitnessComponent>(Elite2, Fit2);

		// 3. Add breeding pair
		auto ParentA = Registry.create();
		FFitnessComponent FitA; FitA.Fitness.Add(50.0f);
		Registry.emplace<FFitnessComponent>(ParentA, FitA);
		
		auto ParentB = Registry.create();
		FFitnessComponent FitB; FitB.Fitness.Add(60.0f);
		Registry.emplace<FFitnessComponent>(ParentB, FitB);

		auto PairEntity = Registry.create();
		FBreedingPairComponent Pair;
		Pair.ParentA = (uint32)ParentA;
		Pair.ParentB = (uint32)ParentB;
		Registry.emplace<FBreedingPairComponent>(PairEntity, Pair);

		// 4. Run system multiple times to check history
		UGADebugDataSystem* DebugSystem = NewObject<UGADebugDataSystem>();
		DebugSystem->Initialize_Implementation(Context);
		
		// History records every 1 second by default. Run for 1.1s.
		DebugSystem->Update_Implementation(1.1f);

		// Verify
		const auto& DebugComp = Registry.get<FGeneticAlgorithmDebugComponent>(DebugEntity);
		
		ASSERT_THAT(AreEqual(2, DebugComp.ResetCount, "ResetCount should be 2"));
		ASSERT_THAT(AreEqual(2, DebugComp.EliteCount, "EliteCount should be 2"));
		ASSERT_THAT(AreEqual(2, DebugComp.EliteFitness.Num(), "EliteFitness should have 2 entries"));
		ASSERT_THAT(IsNear(200.0f, DebugComp.EliteFitness[0], 0.01f, "First elite fitness should be near 200 (sorted)"));
		
		ASSERT_THAT(AreEqual(2, DebugComp.BreedingPairsFitness.Num(), "BreedingPairsFitness should have 2 entries (1 pair)"));
		ASSERT_THAT(IsNear(50.0f, DebugComp.BreedingPairsFitness[0], 0.01f, "Parent A fitness should be near 50"));
		ASSERT_THAT(IsNear(60.0f, DebugComp.BreedingPairsFitness[1], 0.01f, "Parent B fitness should be near 60"));

		ASSERT_THAT(AreEqual(1, DebugComp.HistoricalTotalEliteFitness.Num(), "History Total should have 1 entry after 1.1s"));
		ASSERT_THAT(IsNear(300.0f, DebugComp.HistoricalTotalEliteFitness[0], 0.01f, "Historical total fitness should be sum of elites (100+200)"));

		// Run for another 1s
		DebugSystem->Update_Implementation(1.0f);
		ASSERT_THAT(AreEqual(2, DebugComp.HistoricalTotalEliteFitness.Num(), "History Total should have 2 entries after another 1s"));
	}
};
