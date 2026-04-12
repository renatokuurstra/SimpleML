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

	// 2. Identify entities eligible for FFitnessComponent
	// Criteria:
	// - MinBreedAge met
	// - Age >= OldestAliveAgeFactor * OldestAge
	// - Elite genomes (FEliteTagComponent) always get fitness (they don't reset age, but just in case)
	
	float MinAgeRequired = FMath::Max(Config->MinBreedAge, OldestAge * Config->OldestAliveAgeFactor);

	// We use exclude_t to only find entities that DON'T have FFitnessComponent yet.
	auto IneligibleView = GetRegistry().view<FTrainingDataComponent>(entt::exclude_t<FFitnessComponent, FEliteTagComponent>{});

	for (auto Entity : IneligibleView)
	{
		const FTrainingDataComponent& Data = IneligibleView.get<FTrainingDataComponent>(Entity);
		float Age = CurrentTime - Data.CreationTime;

		if (Age >= MinAgeRequired)
		{
			FFitnessComponent& FitComp = GetRegistry().emplace<FFitnessComponent>(Entity);
			FitComp.Fitness.SetNum(1);
			FitComp.Fitness[0] = 0.0f;
			FitComp.BuiltForFitnessIndex = 0;
		}
	}
}
