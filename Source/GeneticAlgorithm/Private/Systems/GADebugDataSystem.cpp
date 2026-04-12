//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/GADebugDataSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "Components/BreedingPairComponent.h"

UGADebugDataSystem::UGADebugDataSystem()
{
	RegisterComponent<FGeneticAlgorithmDebugComponent>();
	RegisterComponent<FFitnessComponent>();
	RegisterComponent<FEliteTagComponent>();
	RegisterComponent<FBreedingPairComponent>();
	RegisterComponent<FResetGenomeComponent>();
}

void UGADebugDataSystem::Update_Implementation(float DeltaTime)
{
	auto& Registry = GetRegistry();
	auto DebugView = GetView<FGeneticAlgorithmDebugComponent>();

	if (DebugView.begin() == DebugView.end())
	{
		return;
	}

	entt::entity DebugEntity = *DebugView.begin();
	FGeneticAlgorithmDebugComponent& DebugComp = DebugView.get<FGeneticAlgorithmDebugComponent>(DebugEntity);

	// 1. Reset Count
	auto ResetView = GetView<FResetGenomeComponent>();
	if (ResetView.begin() != ResetView.end())
	{
		int32 ResetCount = 0;
		for (auto E : ResetView) { ResetCount++; }
		DebugComp.ResetCount = ResetCount;
	}

	// 2. Elite Info
	auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent>();
	if (EliteView.begin() != EliteView.end())
	{
		int32 EliteCount = 0;
		for (auto E : EliteView) { EliteCount++; }
		DebugComp.EliteCount = EliteCount;
		DebugComp.EliteFitness.Reset();
		for (auto E : EliteView)
		{
			const auto& Fit = EliteView.get<FFitnessComponent>(E);
			if (Fit.Fitness.Num() > 0)
			{
				// Assuming first fitness value is the primary one for visualization
				DebugComp.EliteFitness.Add(Fit.Fitness[0]);
			}
		}
		DebugComp.EliteFitness.Sort(TGreater<float>());
	}

	// 3. Breeding Pairs Info
	auto BreedingView = GetView<FBreedingPairComponent>();
	if (BreedingView.begin() != BreedingView.end())
	{
		DebugComp.BreedingPairsFitness.Reset();
		int32 PairsAdded = 0;
		for (auto E : BreedingView)
		{
			const auto& Breeding = BreedingView.get<FBreedingPairComponent>(E);
			entt::entity ParentA = static_cast<entt::entity>(Breeding.ParentA);
			entt::entity ParentB = static_cast<entt::entity>(Breeding.ParentB);

			float FitA = 0.0f;
			float FitB = 0.0f;

			if (Registry.valid(ParentA) && Registry.any_of<FFitnessComponent>(ParentA))
			{
				const auto& Fit = Registry.get<FFitnessComponent>(ParentA);
				if (Fit.Fitness.Num() > 0) FitA = Fit.Fitness[0];
			}
			if (Registry.valid(ParentB) && Registry.any_of<FFitnessComponent>(ParentB))
			{
				const auto& Fit = Registry.get<FFitnessComponent>(ParentB);
				if (Fit.Fitness.Num() > 0) FitB = Fit.Fitness[0];
			}

			DebugComp.BreedingPairsFitness.Add(FitA);
			DebugComp.BreedingPairsFitness.Add(FitB);
			
			PairsAdded++;
			if (PairsAdded >= 4) break; // As requested, show first 4 (8 values)
		}
	}

	// 4. All Solutions Fitness
	auto PopView = Registry.view<FFitnessComponent>(entt::exclude_t<FEliteTagComponent>{});
	if (PopView.begin() != PopView.end())
	{
		DebugComp.AllSolutionsFitness.Reset();
		for (auto E : PopView)
		{
			const auto& Fit = PopView.get<FFitnessComponent>(E);
			if (Fit.Fitness.Num() > 0)
			{
				DebugComp.AllSolutionsFitness.Add(Fit.Fitness[0]);
			}
		}
		DebugComp.AllSolutionsFitness.Sort(TGreater<float>());
	}
}
