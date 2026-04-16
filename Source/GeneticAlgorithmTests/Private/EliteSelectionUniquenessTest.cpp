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
	UEliteSelectionFloatSystem* EliteSystem = nullptr;

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
		// Create 10 entities with different fitness values
		for (int32 i = 0; i < 10; ++i)
		{
			const entt::entity E = Registry.create();
			FFitnessComponent Fit{};
			Fit.Fitness = { static_cast<float>(i) };
			Fit.BuiltForFitnessIndex = 0;
			Registry.emplace<FFitnessComponent>(E, Fit);
			Registry.emplace<FEligibleForBreedingTagComponent>(E);
			
			// We need a genome for CopyGenomeToElite to work
			FEliteOwnedFloatGenome Genome;
			Genome.Values = { static_cast<float>(i) };
			Registry.emplace<FEliteOwnedFloatGenome>(E, Genome);
			FGenomeFloatViewComponent View;
			View.Values = Genome.Values;
			Registry.emplace<FGenomeFloatViewComponent>(E, View);
		}

		// Run elite selection
		EliteSystem->Update_Implementation(0.0f);

		// Check elites
		auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent>();
		TArray<float> EliteFitnesses;
		for (auto E : EliteView)
		{
			EliteFitnesses.Add(EliteView.get<FFitnessComponent>(E).Fitness[0]);
		}

		// Should have 3 elites
		ASSERT_THAT(AreEqual(3, EliteFitnesses.Num(), "Should have exactly 3 elites"));

		// Should have unique fitnesses (9, 8, 7)
		EliteFitnesses.Sort([](float A, float B) { return A > B; });
		ASSERT_THAT(IsNear(9.0f, EliteFitnesses[0], 0.001f, "Best elite should have fitness 9"));
		ASSERT_THAT(IsNear(8.0f, EliteFitnesses[1], 0.001f, "Second elite should have fitness 8"));
		ASSERT_THAT(IsNear(7.0f, EliteFitnesses[2], 0.001f, "Third elite should have fitness 7"));

		// Now simulate another generation where the best solution improves
		// In a real GA, the population is replaced. But elites persist.
		// Let's just update the population fitness.
		auto PopView = Registry.view<FFitnessComponent>(entt::exclude_t<FEliteTagComponent>{});
		for (auto E : PopView)
		{
			FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
			Fit.Fitness[0] += 10.0f; // New fitnesses 10 to 19
		}

		// Run elite selection again
		EliteSystem->Update_Implementation(0.0f);

		EliteFitnesses.Reset();
		for (auto E : EliteView)
		{
			EliteFitnesses.Add(EliteView.get<FFitnessComponent>(E).Fitness[0]);
		}
		EliteFitnesses.Sort([](float A, float B) { return A > B; });

		ASSERT_THAT(AreEqual(3, EliteFitnesses.Num(), "Should still have 3 elites"));
		ASSERT_THAT(IsNear(19.0f, EliteFitnesses[0], 0.001f, "Best elite should now be 19"));
		ASSERT_THAT(IsNear(18.0f, EliteFitnesses[1], 0.001f, "Second elite should now be 18"));
		ASSERT_THAT(IsNear(17.0f, EliteFitnesses[2], 0.001f, "Third elite should now be 17"));
		
		// Ensure no duplicates in the final elite fitnesses
		TSet<float> UniqueFitnesses;
		for(float F : EliteFitnesses) { UniqueFitnesses.Add(F); }
		ASSERT_THAT(AreEqual(3, UniqueFitnesses.Num(), "Elites should all have unique fitness values from the top of the combined pool"));
	}
};
