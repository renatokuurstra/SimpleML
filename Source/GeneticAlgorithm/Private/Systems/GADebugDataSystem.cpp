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

	// Try to get max history length from context/config if available
	// Since we are in GeneticAlgorithm module, we don't know about VehicleTrainerConfig.
	// We'll rely on the value already set in DebugComp or a default.

	// 1. Reset Count & Iteration Detection
	auto ResetView = GetView<FResetGenomeComponent>();
	int32 CurrentResetCount = 0;
	if (ResetView.begin() != ResetView.end())
	{
		for (auto E : ResetView) { CurrentResetCount++; }
	}
	DebugComp.ResetCount = CurrentResetCount;

	// 2. Elite Info & Historical Fitness
	auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent>();
	float CurrentEliteTotalFitness = 0.0f;
	if (EliteView.begin() != EliteView.end())
	{
		int32 EliteCount = 0;
		DebugComp.EliteFitness.Reset();
		for (auto E : EliteView)
		{
			EliteCount++;
			const auto& Fit = EliteView.get<FFitnessComponent>(E);
			if (Fit.Fitness.Num() > 0)
			{
				float Val = Fit.Fitness[0];
				DebugComp.EliteFitness.Add(Val);
				CurrentEliteTotalFitness += Val;
			}
		}
		DebugComp.EliteCount = EliteCount;
		DebugComp.EliteFitness.Sort(TGreater<float>());
	}
	
	// Record historical elite fitness periodically
	SampleTimer += DeltaTime;
	if (SampleTimer >= SampleInterval)
	{
		SampleTimer = 0.0f;
		
		// Record Total
		DebugComp.HistoricalTotalEliteFitness.Add(CurrentEliteTotalFitness);
		if (DebugComp.HistoricalTotalEliteFitness.Num() > DebugComp.MaxHistoryLength)
		{
			DebugComp.HistoricalTotalEliteFitness.RemoveAt(0);
		}
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
	auto PopView = Registry.view<FFitnessComponent>();
	DebugComp.AllSolutionsFitness.Reset();

	for (auto E : PopView)
	{
		const auto& Fit = PopView.get<FFitnessComponent>(E);
		float CurrentFit = (Fit.Fitness.Num() > 0) ? Fit.Fitness[0] : 0.0f;
		
		if (!Registry.any_of<FEliteTagComponent>(E))
		{
			DebugComp.AllSolutionsFitness.Add(CurrentFit);
		}
	}
	DebugComp.AllSolutionsFitness.Sort(TGreater<float>());
}
