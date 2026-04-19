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

	// Update historical fitness length if changed
	if (TrainerContext->TrainerConfig)
	{
		FGeneticAlgorithmDebugComponent& MutableDebugComp = const_cast<FGeneticAlgorithmDebugComponent&>(DebugComp);
		MutableDebugComp.MaxHistoryLength = TrainerContext->TrainerConfig->FitnessHistoryLength;
	}

	// Update cached data
	CachedEliteCount = DebugComp.EliteCount;
	CachedEliteFitness = DebugComp.EliteFitness;
	CachedBreedingPairsFitness = DebugComp.BreedingPairsFitness;
	CachedAllSolutionsFitness = DebugComp.AllSolutionsFitness;
	CachedResetCount = DebugComp.ResetCount;

	if (DebugComp.PopulationTotalEliteFitness.Num() > 0)
	{
		FString LogStr = FString::Printf(TEXT("EU GA Evaluation: "));
		float OverallTotal = 0.0f;
		
		// Sort keys to ensure consistent logging order
		TArray<int32> PopIndices;
		DebugComp.PopulationTotalEliteFitness.GetKeys(PopIndices);
		PopIndices.Sort();

		for (int32 PopIdx : PopIndices)
		{
			float Val = DebugComp.PopulationTotalEliteFitness[PopIdx];
			LogStr += FString::Printf(TEXT("[Pop %d: %.2f] "), PopIdx, Val);
			OverallTotal += Val;
		}

		FGeneticAlgorithmDebugComponent& MutableDebugComp = const_cast<FGeneticAlgorithmDebugComponent&>(DebugComp);
		MutableDebugComp.HistoricalTotalEliteFitness.Add(OverallTotal);
		if (MutableDebugComp.HistoricalTotalEliteFitness.Num() > MutableDebugComp.MaxHistoryLength)
		{
			MutableDebugComp.HistoricalTotalEliteFitness.RemoveAt(0);
		}

		UE_LOG(LogTemp, Log, TEXT("%s"), *LogStr);
	}
}

void UVehicleTrainerDebugSystem::Deinitialize_Implementation()
{
	Super::Deinitialize_Implementation();
}

void UVehicleTrainerDebugSystem::DrawDebugUI()
{
}
