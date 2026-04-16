//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleTrainerDebugSystem.h"
#include "Components/GenomeComponents.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "SlateIM.h"
#include "TimerManager.h"
#include "Engine/World.h"

UVehicleTrainerDebugSystem::UVehicleTrainerDebugSystem()
{
	RegisterComponent<FGeneticAlgorithmDebugComponent>();
}

void UVehicleTrainerDebugSystem::Initialize_Implementation(AEcsContext* InContext)
{
	Super::Initialize_Implementation(InContext);
}

void UVehicleTrainerDebugSystem::Update_Implementation(float DeltaTime)
{
	AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
	if (!TrainerContext || !TrainerContext->TrainerConfig)
	{
		return;
	}

	auto DebugView = GetView<FGeneticAlgorithmDebugComponent>();
	if (DebugView.begin() == DebugView.end())
	{
		return;
	}

	const FGeneticAlgorithmDebugComponent& DebugComp = DebugView.get<FGeneticAlgorithmDebugComponent>(*DebugView.begin());

	// Update cached data
	CachedEliteCount = DebugComp.EliteCount;
	CachedEliteFitness = DebugComp.EliteFitness;
	CachedBreedingPairsFitness = DebugComp.BreedingPairsFitness;
	CachedAllSolutionsFitness = DebugComp.AllSolutionsFitness;
	CachedResetCount = DebugComp.ResetCount;

	// Update historical fitness length if changed
	if (TrainerContext->TrainerConfig)
	{
		FGeneticAlgorithmDebugComponent& MutableDebugComp = const_cast<FGeneticAlgorithmDebugComponent&>(DebugComp);
		MutableDebugComp.MaxHistoryLength = TrainerContext->TrainerConfig->FitnessHistoryLength;
	}
}

void UVehicleTrainerDebugSystem::Deinitialize_Implementation()
{
	Super::Deinitialize_Implementation();
}

void UVehicleTrainerDebugSystem::DrawDebugUI()
{
}
