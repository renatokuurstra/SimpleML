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

	// 1. Find the oldest currently alive entity (using FTrainingDataComponent)
	float OldestAge = 0.0f;
	auto AllEntitiesView = GetView<FTrainingDataComponent>();

	for (auto Entity : AllEntitiesView)
	{
		const FTrainingDataComponent& Data = AllEntitiesView.get<FTrainingDataComponent>(Entity);
		float Age = CurrentTime - Data.CreationTime;
		if (Age > OldestAge)
		{
			OldestAge = Age;
		}
	}

	// 2. Identify entities eligible for breeding
	// Criteria:
	// - Entity is flagged for reset (FResetGenomeComponent)
	// - MinBreedAge met
	// - Age >= OldestAliveAgeFactor * OldestAge
	// - Elite genomes (FEliteTagComponent) always get fitness (they don't reset age, but just in case)
	// - Reset reason is not blocked for breeding in Config->ResetReasonConfigs
	
	float MinAgeRequired = FMath::Max(Config->MinBreedAge, OldestAge * Config->OldestAliveAgeFactor);

	// We use exclude_t to only find entities that DON'T have FEligibleForBreedingTagComponent yet.
	// We only want to evaluate entities that are flagged for reset.
	auto IneligibleView = GetRegistry().view<FTrainingDataComponent, FResetGenomeComponent, FFitnessComponent>(entt::exclude_t<FEligibleForBreedingTagComponent, FEliteTagComponent>{});

	for (auto Entity : IneligibleView)
	{
		const FTrainingDataComponent& Data = IneligibleView.get<FTrainingDataComponent>(Entity);
		const FResetGenomeComponent& ResetComp = IneligibleView.get<FResetGenomeComponent>(Entity);

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

		if (Age >= MinAgeRequired)
		{
			GetRegistry().emplace<FEligibleForBreedingTagComponent>(Entity);
		}
	}
}
