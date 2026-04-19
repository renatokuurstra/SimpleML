//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleFitnessEligibilitySystem.h"
#include "Components/TrainingDataComponent.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Engine/World.h"

UVehicleFitnessEligibilitySystem::UVehicleFitnessEligibilitySystem()
{
	RegisterComponent<FTrainingDataComponent>();
	RegisterComponent<FFitnessComponent>();
	RegisterComponent<FEligibleForBreedingTagComponent>();
}

void UVehicleFitnessEligibilitySystem::Update_Implementation(float DeltaTime)
{
	AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
	if (!TrainerContext || !TrainerContext->TrainerConfig)
	{
		return;
	}

	float CurrentTime = GetContext()->GetWorld()->GetTimeSeconds();
	UVehicleTrainerConfig* Config = TrainerContext->TrainerConfig;

	// 1. Ensure all elites are marked as eligible.
	// Elites might not have FResetGenomeComponent, but they should always be eligible for breeding.
	auto EliteView = GetRegistry().view<FEliteTagComponent>(entt::exclude_t<FEligibleForBreedingTagComponent>{});
	for (auto Entity : EliteView)
	{
		GetRegistry().emplace<FEligibleForBreedingTagComponent>(Entity);
	}

	// 2. Find the highest fitness overall (including elites and currently alive entities)
	float MaxFitness = -MAX_FLT;
	auto AllFitnessView = GetView<FFitnessComponent>();

	for (auto Entity : AllFitnessView)
	{
		const FFitnessComponent& Fit = AllFitnessView.get<FFitnessComponent>(Entity);
		// Assuming we care about the first fitness index (index 0) for eligibility
		if (Fit.Fitness.Num() > 0)
		{
			if (Fit.Fitness[0] > MaxFitness)
			{
				MaxFitness = Fit.Fitness[0];
			}
		}
	}

	// 3. Identify entities eligible for breeding
	// Criteria:
	// - Entity is flagged for reset (FResetGenomeComponent)
	// - MinBreedAge met
	// - Fitness >= HighestFitnessFactor * MaxFitness
	// - Reset reason is not blocked for breeding in Config->ResetReasonConfigs
	
	// If MaxFitness is still -MAX_FLT, it means no one has fitness yet.
	// In that case, we might fall back to allowing anyone who met MinBreedAge?
	// The requirement says "solutions with at least that much", which implies fitness comparison.
	// If MaxFitness is 0 (initial), then anyone with fitness >= 0 is eligible.
	
	// We use exclude_t to only find entities that DON'T have FEligibleForBreedingTagComponent yet.
	// We only want to evaluate entities that are flagged for reset.
	auto IneligibleView = GetRegistry().view<FTrainingDataComponent, FResetGenomeComponent, FFitnessComponent>(entt::exclude_t<FEligibleForBreedingTagComponent, FEliteTagComponent>{});

	for (auto Entity : IneligibleView)
	{
		const FTrainingDataComponent& Data = IneligibleView.get<FTrainingDataComponent>(Entity);
		const FResetGenomeComponent& ResetComp = IneligibleView.get<FResetGenomeComponent>(Entity);
		const FFitnessComponent& FitComp = IneligibleView.get<FFitnessComponent>(Entity);

		// Check if the reason for reset blocks breeding
		bool bIsBlocked = false;
		for (const FResetReasonConfig& ReasonConfig : Config->ResetReasonConfigs)
		{
			if (ReasonConfig.Reason == ResetComp.ReasonForReset && ReasonConfig.bBlockBreed)
			{
				bIsBlocked = true;
				break;
			}
		}

		if (bIsBlocked)
		{
			continue;
		}

		float Age = CurrentTime - Data.CreationTime;
		
		if (Age < Config->MinBreedAge)
		{
			continue;
		}

		float EntityFitness = (FitComp.Fitness.Num() > 0) ? FitComp.Fitness[0] : -MAX_FLT;

		// Eligibility check:
		// If MaxFitness is still very low, we might want a minimum threshold or just let them through if they lived long enough.
		// But the prompt says "at least that much" of the highest fitness.
		bool bPassFitnessThreshold = true;
		if (MaxFitness > -MAX_FLT)
		{
			bPassFitnessThreshold = (EntityFitness >= MaxFitness * Config->HighestFitnessFactor);
		}

		if (bPassFitnessThreshold)
		{
			GetRegistry().emplace<FEligibleForBreedingTagComponent>(Entity);
		}
	}

	// 4. Mark good non-reset, non-elite entities as eligible parents.
	// Active vehicles performing well should be tournament candidates, not just elites.
	auto ActiveView = GetRegistry().view<FTrainingDataComponent, FFitnessComponent>(
		entt::exclude_t<FResetGenomeComponent, FEligibleForBreedingTagComponent, FEliteTagComponent>{});

	for (auto Entity : ActiveView)
	{
		const FTrainingDataComponent& Data = ActiveView.get<FTrainingDataComponent>(Entity);
		const FFitnessComponent& FitComp = ActiveView.get<FFitnessComponent>(Entity);

		float Age = CurrentTime - Data.CreationTime;
		if (Age < Config->MinBreedAge)
		{
			continue;
		}

		float EntityFitness = (FitComp.Fitness.Num() > 0) ? FitComp.Fitness[0] : -MAX_FLT;

		bool bPassActive = true;
		if (MaxFitness > -MAX_FLT)
		{
			bPassActive = (EntityFitness >= MaxFitness * Config->HighestFitnessFactor);
		}

		if (bPassActive)
		{
			GetRegistry().emplace<FEligibleForBreedingTagComponent>(Entity);
		}
	}
}
