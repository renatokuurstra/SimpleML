// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "entt/entt.hpp"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "Systems/EliteSelectionFloatSystem.h"

TEST_CLASS(EliteSelection_Uniqueness_Tests, "GeneticAlgorithm.EliteSelection")
{
	entt::registry Registry;
	TObjectPtr<UEliteSelectionFloatSystem> EliteSystem;

	BEFORE_EACH()
	{
		EliteSystem = NewObject<UEliteSelectionFloatSystem>();
		IEcsEventElement::Execute_Initialize(EliteSystem, nullptr);
		EliteSystem->EliteCount = 3;
		EliteSystem->bHigherIsBetter = true;
	}

	AFTER_EACH()
	{
		if (EliteSystem)
		{
			IEcsEventElement::Execute_Deinitialize(EliteSystem);
			EliteSystem = nullptr;
		}
		Registry.clear();
	}

	TEST_METHOD(A_Single_Solution_Does_Not_Take_Multiple_Elite_Spots)
	{
		// 1. Create 10 entities with unique IDs and different fitness values
		TArray<int64> UniqueIds;
		for (int32 i = 0; i < 10; ++i)
		{
			const entt::entity E = Registry.create();
			FFitnessComponent Fit{};
			Fit.Fitness.Add(static_cast<float>(i));
			Fit.BuiltForFitnessIndex = 0;
			Registry.emplace<FFitnessComponent>(E, MoveTemp(Fit));
			Registry.emplace<FEligibleForBreedingTagComponent>(E);
			
			FUniqueSolutionComponent Unique;
			Unique.Id = FUniqueSolutionComponent::GenerateNewId();
			UniqueIds.Add(Unique.Id);
			Registry.emplace<FUniqueSolutionComponent>(E, Unique);

			FEliteOwnedFloatGenome Genome;
			Genome.Values.Add(static_cast<float>(i));
			Registry.emplace<FEliteOwnedFloatGenome>(E, MoveTemp(Genome));
			FGenomeFloatViewComponent View;
			View.Values = Registry.get<FEliteOwnedFloatGenome>(E).Values;
			Registry.emplace<FGenomeFloatViewComponent>(E, View);
		}

		// Run elite selection
		EliteSystem->Update_Implementation(0.0f);

		// Check elites
		auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent, FUniqueSolutionComponent>();
		int32 EliteCount = 0;
		for (auto E : EliteView) { ++EliteCount; }
		ASSERT_THAT(AreEqual((int32)3, EliteCount, TEXT("Should have exactly 3 elites")));

		TArray<float> EliteFitnesses;
		TArray<int64> EliteSourceIds;
		for (auto E : EliteView)
		{
			EliteFitnesses.Add(EliteView.get<FFitnessComponent>(E).Fitness[0]);
			EliteSourceIds.Add(EliteView.get<FUniqueSolutionComponent>(E).SourceId);
		}

		EliteFitnesses.Sort([](float A, float B) { return A > B; });
		ASSERT_THAT(IsNear(9.0f, EliteFitnesses[0], 0.001f, TEXT("Best elite should have fitness 9")));
		ASSERT_THAT(IsNear(8.0f, EliteFitnesses[1], 0.001f, TEXT("Second elite should have fitness 8")));
		ASSERT_THAT(IsNear(7.0f, EliteFitnesses[2], 0.001f, TEXT("Third elite should have fitness 7")));

		// 2. Simulate another generation where IDs stay the same but fitness changes
		// AND we have multiple entities with the SAME ID (duplicates)
		auto PopView = Registry.view<FFitnessComponent, FUniqueSolutionComponent>(entt::exclude_t<FEliteTagComponent>{});
		for (auto E : PopView)
		{
			FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
			// Update fitness: 9->19, 8->18, etc.
			Fit.Fitness[0] += 10.0f; 

			// Create a duplicate entity with the same ID but lower fitness
			const entt::entity Duplicate = Registry.create();
			FFitnessComponent DupFit = Fit;
			DupFit.Fitness[0] -= 5.0f; // Lower fitness for the same solution
			Registry.emplace<FFitnessComponent>(Duplicate, DupFit);
			Registry.emplace<FEligibleForBreedingTagComponent>(Duplicate);
			Registry.emplace<FUniqueSolutionComponent>(Duplicate, Registry.get<FUniqueSolutionComponent>(E));
			
			// Add required genome view
			FGenomeFloatViewComponent& SrcView = Registry.get<FGenomeFloatViewComponent>(E);
			Registry.emplace<FGenomeFloatViewComponent>(Duplicate, SrcView);
		}

		// Run elite selection again
		EliteSystem->Update_Implementation(0.0f);

		EliteFitnesses.Reset();
		EliteSourceIds.Reset();
		auto NewEliteView = Registry.view<FEliteTagComponent, FFitnessComponent, FUniqueSolutionComponent>();
		int32 NewEliteCount = 0;
		for (auto E : NewEliteView)
		{
			++NewEliteCount;
			EliteFitnesses.Add(NewEliteView.get<FFitnessComponent>(E).Fitness[0]);
			EliteSourceIds.Add(NewEliteView.get<FUniqueSolutionComponent>(E).SourceId);
		}
		EliteFitnesses.Sort([](float A, float B) { return A > B; });

		ASSERT_THAT(AreEqual((int32)3, NewEliteCount, TEXT("Should still have 3 elites")));
		ASSERT_THAT(IsNear(19.0f, EliteFitnesses[0], 0.001f, TEXT("Best elite should now be 19")));
		ASSERT_THAT(IsNear(18.0f, EliteFitnesses[1], 0.001f, TEXT("Second elite should now be 18")));
		ASSERT_THAT(IsNear(17.0f, EliteFitnesses[2], 0.001f, TEXT("Third elite should now be 17")));
		
		// Ensure no duplicates in the final elite SOURCE IDs
		TSet<int64> UniqueSourceIds;
		for(int64 SId : EliteSourceIds) { UniqueSourceIds.Add(SId); }
		ASSERT_THAT(AreEqual((int32)3, UniqueSourceIds.Num(), TEXT("Elites should represent 3 unique solution IDs")));
	}
};
