//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/GAStalenessSystem.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "Components/NetworkComponent.h"

UGAStalenessSystem::UGAStalenessSystem()
{
	RegisterComponent<FFitnessComponent>();
	RegisterComponent<FEliteTagComponent>();
	RegisterComponent<FResetGenomeComponent>();
	RegisterComponent<FGenomeFloatViewComponent>();
	RegisterComponent<FNeuralNetworkFloat>();
}

void UGAStalenessSystem::Update_Implementation(float DeltaTime)
{
	AVehicleTrainerContext* TrainerContext = Cast<AVehicleTrainerContext>(GetContext());
	if (!TrainerContext || !TrainerContext->TrainerConfig || !TrainerContext->TrainerConfig->bEnableNuke)
	{
		return;
	}

	const UVehicleTrainerConfig* Config = TrainerContext->TrainerConfig;

	// Staleness system only makes sense with multiple populations
	if (Config->NumPopulations <= 1)
	{
		return;
	}

	auto& Registry = GetRegistry();
	const float CurrentTime = TrainerContext->GetWorld()->GetTimeSeconds();
	const bool bHigherIsBetter = Config->bHigherIsBetter;

	// -----------------------------------------------------------------------
	// 0. Global cooldown -- no nuke allowed until timer expires
	// -----------------------------------------------------------------------
	if (CurrentTime < TrainerContext->NextNukeAvailableTime)
	{
		return;
	}

	// -----------------------------------------------------------------------
	// 1. Gather total elite fitness per population + find global best elite
	// -----------------------------------------------------------------------
	TMap<int32, float> CurrentPopFitness;
	auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent>();

	entt::entity GlobalBestElite = entt::null;
	float GlobalBestFitness = bHigherIsBetter ? -MAX_FLT : MAX_FLT;

	for (auto E : EliteView)
	{
		const auto& Fit = EliteView.get<FFitnessComponent>(E);
		const int32 PopIdx = Fit.BuiltForFitnessIndex;
		if (PopIdx >= 0 && PopIdx < Fit.Fitness.Num())
		{
			const float Val = Fit.Fitness[PopIdx];

			// Only count non-neutral fitness
			const float NeutralVal = bHigherIsBetter ? -MAX_FLT : MAX_FLT;
			if (Val != NeutralVal)
			{
				float& Total = CurrentPopFitness.FindOrAdd(PopIdx);
				Total += Val;

				if (bHigherIsBetter ? (Val > GlobalBestFitness) : (Val < GlobalBestFitness))
				{
					GlobalBestFitness = Val;
					GlobalBestElite = E;
				}
			}
		}
	}

	// Ensure all populations are accounted for (even if they have no valid elites)
	for (int32 i = 0; i < Config->NumPopulations; ++i)
	{
		if (!CurrentPopFitness.Contains(i))
		{
			CurrentPopFitness.Add(i, 0.0f);
		}
	}

	// -----------------------------------------------------------------------
	// 2. Find the population with the LOWEST total elite fitness
	// -----------------------------------------------------------------------
	int32 LowestPopIdx = -1;
	float LowestPopFitness = bHigherIsBetter ? MAX_FLT : -MAX_FLT;

	for (const auto& It : CurrentPopFitness)
	{
		const float Fit = It.Value;
		if (bHigherIsBetter ? (Fit < LowestPopFitness) : (Fit > LowestPopFitness))
		{
			LowestPopFitness = Fit;
			LowestPopIdx = It.Key;
		}
	}

	if (LowestPopIdx == -1)
	{
		return;
	}

	// -----------------------------------------------------------------------
	// 3. Record history for ALL populations (needed for future staleness checks)
	// -----------------------------------------------------------------------
	{
		for (const auto& It : CurrentPopFitness)
		{
			TArray<float>& History = PopulationFitnessHistory.FindOrAdd(It.Key);
			History.Add(It.Value);

			const int32 MaxHistory = Config->MinHistoryForStaleness;
			while (History.Num() > MaxHistory)
			{
				History.RemoveAt(0);
			}
		}
	}

	// -----------------------------------------------------------------------
	// 4. Check staleness ONLY for the lowest-fitness population
	// -----------------------------------------------------------------------
	TArray<float>& LowestHistory = PopulationFitnessHistory.FindOrAdd(LowestPopIdx);

	if (LowestHistory.Num() < Config->MinHistoryForStaleness)
	{
		// Not enough data yet -- just keep collecting
		return;
	}

	float StartFitness = LowestHistory[0];
	float EndFitness = LowestHistory.Last();
	float Improvement = (StartFitness > 0.0001f)
		? (EndFitness - StartFitness) / StartFitness
		: (EndFitness - StartFitness);

	if (Improvement >= Config->StalenessThreshold)
	{
		// Not stale -- still improving
		return;
	}

	// -----------------------------------------------------------------------
	// 5. STALE -- NUKE the lowest-fitness population
	// -----------------------------------------------------------------------
	UE_LOG(LogTemp, Warning,
		TEXT("[GAStalenessSystem] Staleness detected! Nuking lowest-fitness population %d "
			"(TotalFitness: %f, Improvement: %f over %d iterations)"),
		LowestPopIdx, LowestPopFitness, Improvement, Config->MinHistoryForStaleness);

	// 5a. Tag ALL non-elite entities in this population for reset
	{
		auto PopView = Registry.view<FFitnessComponent>(entt::exclude_t<FEliteTagComponent>{});
		for (auto E : PopView)
		{
			const auto& Fit = PopView.get<FFitnessComponent>(E);
			if (Fit.BuiltForFitnessIndex == LowestPopIdx)
			{
				Registry.get_or_emplace<FResetGenomeComponent>(E).ReasonForReset = TEXT("StalenessNuke");
			}
		}
	}

	// 5b. Re-randomize NN weights for ALL non-elite entities in this population
	{
		TArray<FNeuralNetworkLayerDescriptor> LayerDescriptors = Config->GetNNLayerDescriptors();
		auto PopView = Registry.view<FFitnessComponent, FNeuralNetworkFloat>(entt::exclude_t<FEliteTagComponent>{});

		for (auto E : PopView)
		{
			const auto& Fit = PopView.get<FFitnessComponent>(E);
			if (Fit.BuiltForFitnessIndex == LowestPopIdx)
			{
				FNeuralNetworkFloat& NetComp = PopView.get<FNeuralNetworkFloat>(E);
				// Reinitialize with a random seed to get fresh random weights
				const int32 RandomSeed = FMath::RandRange(1, 2147483647);
				NetComp.Initialize(LayerDescriptors, RandomSeed);

				// Update the genome view to point to the reinitialized network data
				if (FGenomeFloatViewComponent* GenomeView = Registry.try_get<FGenomeFloatViewComponent>(E))
				{
					GenomeView->Values = NetComp.Network.GetDataView();
				}
			}
		}
	}

	// 5c. Find the global best elite's genome BEFORE destroying any elites
	const FFitnessComponent* GlobalBestFit = Registry.try_get<FFitnessComponent>(GlobalBestElite);
	TArray<float> BestGenomeCopy;

	if (Registry.valid(GlobalBestElite) && Registry.all_of<FEliteOwnedFloatGenome>(GlobalBestElite))
	{
		const auto& SrcOwned = Registry.get<FEliteOwnedFloatGenome>(GlobalBestElite);
		BestGenomeCopy = SrcOwned.Values;
	}

	// 5d. Destroy ALL elite entities in the nuked population
	{
		TArray<entt::entity> ElitesToDestroy;
		for (auto E : EliteView)
		{
			const auto& Fit = EliteView.get<FFitnessComponent>(E);
			if (Fit.BuiltForFitnessIndex == LowestPopIdx)
			{
				ElitesToDestroy.Add(E);
			}
		}

		for (auto E : ElitesToDestroy)
		{
			Registry.destroy(E);
		}
	}

	// 5e. Create new elite entities for ALL elite slots, seeded with the global best genome
	if (BestGenomeCopy.Num() > 0)
	{
		// Grab the SourceId of the global best elite so we can reference it
		int64 GlobalBestSourceId = 0;
		if (Registry.valid(GlobalBestElite) && Registry.all_of<FUniqueSolutionComponent>(GlobalBestElite))
		{
			GlobalBestSourceId = Registry.get<FUniqueSolutionComponent>(GlobalBestElite).SourceId;
		}

		for (int32 i = 0; i < Config->EliteCount; ++i)
		{
			const entt::entity NewElite = Registry.create();

			Registry.emplace<FEliteTagComponent>(NewElite);

			// CRITICAL: Must have FUniqueSolutionComponent or the elite selection system's
			// EliteView (which requires FEliteTagComponent + FFitnessComponent + FUniqueSolutionComponent)
			// will not see this entity, causing duplicate elites to be created every tick.
			FUniqueSolutionComponent& NewUnique = Registry.emplace<FUniqueSolutionComponent>(NewElite);
			NewUnique.Id = FUniqueSolutionComponent::GenerateNewId();
			NewUnique.SourceId = GlobalBestSourceId;

			FFitnessComponent NewFit{};
			NewFit.BuiltForFitnessIndex = LowestPopIdx;
			NewFit.Fitness.SetNumZeroed(Config->NumPopulations);
			// Set fitness to the actual global best value so these elites aren't immediately
			// overridden by elite selection — they already represent the best known solution.
			NewFit.Fitness[LowestPopIdx] = GlobalBestFitness;
			Registry.emplace<FFitnessComponent>(NewElite, MoveTemp(NewFit));

			FEliteOwnedFloatGenome& NewOwned = Registry.emplace<FEliteOwnedFloatGenome>(NewElite);
			NewOwned.Values = BestGenomeCopy;

			FGenomeFloatViewComponent& NewView = Registry.emplace<FGenomeFloatViewComponent>(NewElite);
			NewView.Values = TArrayView<float>(NewOwned.Values.GetData(), NewOwned.Values.Num());
		}

		UE_LOG(LogTemp, Log,
			TEXT("[GAStalenessSystem] Replaced all %d elite slots in population %d with global best genome "
				"(GlobalBestFitness: %f, from population %d)"),
			Config->EliteCount, LowestPopIdx, GlobalBestFitness,
			GlobalBestFit ? GlobalBestFit->BuiltForFitnessIndex : -1);
	}

	// 5f. Set global cooldown and clear history for the nuked population
	TrainerContext->NextNukeAvailableTime = CurrentTime + Config->StalenessCooldown;
	PopulationFitnessHistory.FindOrAdd(LowestPopIdx).Empty();
}
