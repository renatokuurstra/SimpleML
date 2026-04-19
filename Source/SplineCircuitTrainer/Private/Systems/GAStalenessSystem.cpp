//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/GAStalenessSystem.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"

UGAStalenessSystem::UGAStalenessSystem()
{
	RegisterComponent<FFitnessComponent>();
	RegisterComponent<FEliteTagComponent>();
	RegisterComponent<FResetGenomeComponent>();
	RegisterComponent<FGenomeFloatViewComponent>();
}

void UGAStalenessSystem::Update_Implementation(float DeltaTime)
{
	AVehicleTrainerContext* TrainerContext = Cast<AVehicleTrainerContext>(GetContext());
	if (!TrainerContext || !TrainerContext->TrainerConfig || !TrainerContext->TrainerConfig->bEnableNuke)
	{
		return;
	}

	auto& Registry = GetRegistry();
	const UVehicleTrainerConfig* Config = TrainerContext->TrainerConfig;
	const float CurrentTime = TrainerContext->GetWorld()->GetTimeSeconds();
	const bool bHigherIsBetter = Config->bHigherIsBetter;

	// 1. Gather total elite fitness per population
	TMap<int32, float> CurrentPopFitness;
	auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent>();
	
	entt::entity GlobalBestElite = entt::null;
	float GlobalBestFitness = -MAX_FLT;

	for (auto E : EliteView)
	{
		const auto& Fit = EliteView.get<FFitnessComponent>(E);
		if (Fit.Fitness.Num() > 0)
		{
			float Val = Fit.Fitness[0];
			float& Total = CurrentPopFitness.FindOrAdd(Fit.BuiltForFitnessIndex);
			Total += Val;

			if (Val > GlobalBestFitness)
			{
				GlobalBestFitness = Val;
				GlobalBestElite = E;
			}
		}
	}

	// 2. Evaluate staleness per population
	TArray<int32> StalePopulations;
	for (auto& It : CurrentPopFitness)
	{
		int32 PopIdx = It.Key;
		float TotalFitness = It.Value;

		// Check cooldown
		if (TrainerContext->NextNukeAvailableTimePerPopulation.Contains(PopIdx) && CurrentTime < TrainerContext->NextNukeAvailableTimePerPopulation[PopIdx])
		{
			continue;
		}

		TArray<float>& History = PopulationFitnessHistory.FindOrAdd(PopIdx);
		History.Add(TotalFitness);

		if (History.Num() >= Config->MinHistoryForStaleness)
		{
			if (History.Num() > Config->MinHistoryForStaleness)
			{
				History.RemoveAt(0);
			}

			float StartFitness = History[0];
			float EndFitness = History.Last();
			float Improvement = (StartFitness > 0.0001f) ? (EndFitness - StartFitness) / StartFitness : (EndFitness - StartFitness);

			if (Improvement < Config->StalenessThreshold)
			{
				StalePopulations.Add(PopIdx);
			}
		}
	}

	if (StalePopulations.Num() == 0)
	{
		return;
	}

	// 3. Find the worst stale population (lowest total elite fitness)
	int32 WorstStalePop = -1;
	float MinStaleFitness = MAX_FLT;

	for (int32 PopIdx : StalePopulations)
	{
		float Fit = CurrentPopFitness[PopIdx];
		if (Fit < MinStaleFitness)
		{
			MinStaleFitness = Fit;
			WorstStalePop = PopIdx;
		}
	}

	if (WorstStalePop != -1)
	{
		// 4. NUKE IT
		UE_LOG(LogTemp, Warning, TEXT("[GAStalenessSystem] Nuking stale population %d (Fitness: %f)"), WorstStalePop, MinStaleFitness);

		// Tag all non-elite entities in this population for reset
		auto PopView = GetRegistry().view<FFitnessComponent>(entt::exclude_t<FEliteTagComponent>{});
		for (auto E : PopView)
		{
			const auto& Fit = PopView.get<FFitnessComponent>(E);
			if (Fit.BuiltForFitnessIndex == WorstStalePop)
			{
				GetRegistry().get_or_emplace<FResetGenomeComponent>(E).ReasonForReset = TEXT("StalenessNuke");
			}
		}

		// Pass 1: find worst elite in stale pop BEFORE resetting (so comparison is meaningful)
		auto ElitePopView = GetRegistry().view<FEliteTagComponent, FFitnessComponent>();
		entt::entity LocalWorstElite = entt::null;
		float LocalWorstFitness = bHigherIsBetter ? MAX_FLT : -MAX_FLT;

		for (auto E : ElitePopView)
		{
			const auto& Fit = ElitePopView.get<FFitnessComponent>(E);
			if (Fit.BuiltForFitnessIndex != WorstStalePop || Fit.Fitness.Num() == 0)
			{
				continue;
			}
			const float F = Fit.Fitness[0];
			const bool bWorse = bHigherIsBetter ? (F < LocalWorstFitness) : (F > LocalWorstFitness);
			if (LocalWorstElite == entt::null || bWorse)
			{
				LocalWorstFitness = F;
				LocalWorstElite = E;
			}
		}

		// Pass 2: reset all elite fitness in stale pop to neutral
		for (auto E : ElitePopView)
		{
			auto& Fit = ElitePopView.get<FFitnessComponent>(E);
			if (Fit.BuiltForFitnessIndex == WorstStalePop && Fit.Fitness.Num() > 0)
			{
				Fit.Fitness[0] = bHigherIsBetter ? -MAX_FLT : MAX_FLT;
			}
		}

		// 5. Pioneer Injection: create a new elite entity seeded with the global best genome.
		// Only migrate from a DIFFERENT population so we don't clone within the same pool.
		const FFitnessComponent* GlobalBestFit = GetRegistry().try_get<FFitnessComponent>(GlobalBestElite);
		const bool bGlobalBestFromDifferentPop = GlobalBestFit && GlobalBestFit->BuiltForFitnessIndex != WorstStalePop;

		if (GetRegistry().valid(GlobalBestElite) && GetRegistry().valid(LocalWorstElite) && bGlobalBestFromDifferentPop)
		{
			if (GetRegistry().all_of<FEliteOwnedFloatGenome>(GlobalBestElite))
			{
				const auto& SrcOwned = GetRegistry().get<FEliteOwnedFloatGenome>(GlobalBestElite);

				// Destroy the worst elite slot to maintain EliteCount, then create a fresh pioneer entity
				GetRegistry().destroy(LocalWorstElite);

				const entt::entity Pioneer = GetRegistry().create();
				GetRegistry().emplace<FEliteTagComponent>(Pioneer);

				FFitnessComponent PioneerFit{};
				PioneerFit.BuiltForFitnessIndex = WorstStalePop;
				PioneerFit.Fitness.SetNum(WorstStalePop + 1, EAllowShrinking::No);
				PioneerFit.Fitness[WorstStalePop] = bHigherIsBetter ? -MAX_FLT : MAX_FLT;
				GetRegistry().emplace<FFitnessComponent>(Pioneer, MoveTemp(PioneerFit));

				FEliteOwnedFloatGenome& PioneerOwned = GetRegistry().emplace<FEliteOwnedFloatGenome>(Pioneer);
				PioneerOwned.Values = SrcOwned.Values;

				FGenomeFloatViewComponent& PioneerView = GetRegistry().emplace<FGenomeFloatViewComponent>(Pioneer);
				PioneerView.Values = TArrayView<float>(PioneerOwned.Values.GetData(), PioneerOwned.Values.Num());

				UE_LOG(LogTemp, Log, TEXT("[GAStalenessSystem] Pioneer migrated from population %d into population %d"),
					GlobalBestFit->BuiltForFitnessIndex, WorstStalePop);
			}
		}

		// Set cooldown
		TrainerContext->NextNukeAvailableTimePerPopulation.FindOrAdd(WorstStalePop) = CurrentTime + Config->StalenessCooldown;
		PopulationFitnessHistory.FindOrAdd(WorstStalePop).Empty();
	}
}
