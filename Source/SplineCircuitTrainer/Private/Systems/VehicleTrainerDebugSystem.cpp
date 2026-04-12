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
	
	if (UWorld* World = GetContext()->GetWorld())
	{
		World->GetTimerManager().SetTimer(DebugTimerHandle, this, &UVehicleTrainerDebugSystem::DrawDebugUI, 0.001f, true);
	}
}

void UVehicleTrainerDebugSystem::Update_Implementation(float DeltaTime)
{
	AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
	if (!TrainerContext || !TrainerContext->TrainerConfig || !TrainerContext->TrainerConfig->bDebugInfo)
	{
		// Stop timer if it's running and debug is disabled
		if (DebugTimerHandle.IsValid())
		{
			if (UWorld* World = GetContext()->GetWorld())
			{
				World->GetTimerManager().ClearTimer(DebugTimerHandle);
			}
		}
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

	// Ensure timer is running
	if (!DebugTimerHandle.IsValid())
	{
		if (UWorld* World = GetContext()->GetWorld())
		{
			World->GetTimerManager().SetTimer(DebugTimerHandle, this, &UVehicleTrainerDebugSystem::DrawDebugUI, 0.001f, true);
		}
	}
}

void UVehicleTrainerDebugSystem::Deinitialize_Implementation()
{
	if (DebugTimerHandle.IsValid())
	{
		if (UWorld* World = GetContext()->GetWorld())
		{
			World->GetTimerManager().ClearTimer(DebugTimerHandle);
		}
	}

	Super::Deinitialize_Implementation();
}

void UVehicleTrainerDebugSystem::DrawDebugUI()
{
	if (!SlateIM::BeginWindowRoot(TEXT("VehicleTrainerDebug"), TEXT("Vehicle Trainer Debug"), FVector2f(400.0f, 600.0f), true))
	{
		SlateIM::EndRoot();
		return;
	}

	SlateIM::BeginScrollBox();
	{
		SlateIM::Text(TEXT("Genetic Algorithm Debug"), FLinearColor::Yellow);
		SlateIM::Spacer(FVector2D(0, 10));

		// 1. Reset Count
		SlateIM::Text(FString::Printf(TEXT("Reset Count: %d"), CachedResetCount));
		SlateIM::Spacer(FVector2D(0, 10));

		// 2. Elites
		SlateIM::Text(FString::Printf(TEXT("Elites: %d"), CachedEliteCount));
		
		SlateIM::FixedTableColumnWidth(60.0f);
		SlateIM::AddTableColumn(TEXT("Index"));
		SlateIM::InitialTableColumnWidth(100.0f);
		SlateIM::AddTableColumn(TEXT("Fitness"));
		
		SlateIM::MaxHeight(150.0f);
		SlateIM::BeginTable();
		{
			// We always show at least the number of elites configured in the trainer, even if not yet populated
			AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
			int32 ConfiguredElites = (TrainerContext && TrainerContext->TrainerConfig) ? TrainerContext->TrainerConfig->EliteCount : 1;
			int32 RowsToShow = FMath::Max(CachedEliteFitness.Num(), ConfiguredElites);

			for (int32 i = 0; i < RowsToShow; ++i)
			{
				if (SlateIM::NextTableCell())
				{
					if (i < CachedEliteFitness.Num()) SlateIM::Text(FString::FromInt(i));
					else SlateIM::Text(TEXT("-"));
				}
				if (SlateIM::NextTableCell())
				{
					if (i < CachedEliteFitness.Num()) SlateIM::Text(FString::SanitizeFloat(CachedEliteFitness[i]));
					else SlateIM::Text(TEXT("-"));
				}
			}
		}
		SlateIM::EndTable();
		SlateIM::Spacer(FVector2D(0, 10));

		// 3. Breeding Pairs
		SlateIM::Text(TEXT("Breeding (First 4 Pairs):"));
		
		SlateIM::FixedTableColumnWidth(40.0f);
		SlateIM::AddTableColumn(TEXT("Pair"));
		SlateIM::InitialTableColumnWidth(100.0f);
		SlateIM::AddTableColumn(TEXT("Parent A"));
		SlateIM::InitialTableColumnWidth(100.0f);
		SlateIM::AddTableColumn(TEXT("Parent B"));
		
		SlateIM::BeginTable();
		{
			int32 PairCount = CachedBreedingPairsFitness.Num() / 2;
			int32 RowsToShow = FMath::Max(PairCount, 1);
			for (int32 i = 0; i < RowsToShow; ++i)
			{
				if (SlateIM::NextTableCell())
				{
					if (i < PairCount) SlateIM::Text(FString::FromInt(i));
					else SlateIM::Text(TEXT("-"));
				}
				if (SlateIM::NextTableCell())
				{
					if (i < PairCount) SlateIM::Text(FString::SanitizeFloat(CachedBreedingPairsFitness[i * 2]));
					else SlateIM::Text(TEXT("-"));
				}
				if (SlateIM::NextTableCell())
				{
					if (i < PairCount) SlateIM::Text(FString::SanitizeFloat(CachedBreedingPairsFitness[i * 2 + 1]));
					else SlateIM::Text(TEXT("-"));
				}
			}
		}
		SlateIM::EndTable();
		SlateIM::Spacer(FVector2D(0, 10));

		// 4. All Solutions Fitness
		SlateIM::Text(TEXT("All Solutions:"));
		
		SlateIM::FixedTableColumnWidth(60.0f);
		SlateIM::AddTableColumn(TEXT("Rank"));
		SlateIM::InitialTableColumnWidth(100.0f);
		SlateIM::AddTableColumn(TEXT("Fitness"));
		
		SlateIM::MaxHeight(200.0f);
		SlateIM::BeginTable();
		{
			int32 Total = CachedAllSolutionsFitness.Num();
			// Since everything is scrollable, we can show a larger number or fixed placeholder
			AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
			int32 PopulationSize = (TrainerContext && TrainerContext->TrainerConfig) ? TrainerContext->TrainerConfig->Population : 10;
			int32 RowsToShow = FMath::Max(Total, PopulationSize); 
			for (int32 i = 0; i < RowsToShow; ++i)
			{
				if (SlateIM::NextTableCell())
				{
					if (i < Total) SlateIM::Text(FString::FromInt(i));
					else SlateIM::Text(TEXT("-"));
				}
				if (SlateIM::NextTableCell())
				{
					if (i < Total) SlateIM::Text(FString::SanitizeFloat(CachedAllSolutionsFitness[i]));
					else SlateIM::Text(TEXT("-"));
				}
			}
		}
		SlateIM::EndTable();
	}
	SlateIM::EndScrollBox();

	SlateIM::EndRoot();
}
