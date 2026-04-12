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
		World->GetTimerManager().SetTimer(DebugTimerHandle, this, &UVehicleTrainerDebugSystem::DrawDebugUI, 0.016f, true);
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
			World->GetTimerManager().SetTimer(DebugTimerHandle, this, &UVehicleTrainerDebugSystem::DrawDebugUI, 0.016f, true);
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
		SlateIM::BeginTable();
		SlateIM::AddTableColumn(TEXT("Index"));
		SlateIM::AddTableColumn(TEXT("Fitness"));
		for (int32 i = 0; i < CachedEliteFitness.Num(); ++i)
		{
			if (SlateIM::NextTableCell()) SlateIM::Text(FString::FromInt(i));
			if (SlateIM::NextTableCell()) SlateIM::Text(FString::SanitizeFloat(CachedEliteFitness[i]));
		}
		SlateIM::EndTable();
		SlateIM::Spacer(FVector2D(0, 10));

		// 3. Breeding Pairs
		SlateIM::Text(TEXT("Breeding (First 4 Pairs):"));
		SlateIM::BeginTable();
		SlateIM::AddTableColumn(TEXT("Pair"));
		SlateIM::AddTableColumn(TEXT("Parent A"));
		SlateIM::AddTableColumn(TEXT("Parent B"));
		for (int32 i = 0; i < CachedBreedingPairsFitness.Num() / 2; ++i)
		{
			if (SlateIM::NextTableCell()) SlateIM::Text(FString::FromInt(i));
			if (SlateIM::NextTableCell()) SlateIM::Text(FString::SanitizeFloat(CachedBreedingPairsFitness[i * 2]));
			if (SlateIM::NextTableCell()) SlateIM::Text(FString::SanitizeFloat(CachedBreedingPairsFitness[i * 2 + 1]));
		}
		SlateIM::EndTable();
		SlateIM::Spacer(FVector2D(0, 10));

		// 4. All Solutions Fitness
		SlateIM::Text(TEXT("All Solutions:"));
		int32 Total = CachedAllSolutionsFitness.Num();
		if (Total > 0)
		{
			SlateIM::BeginTable();
			SlateIM::AddTableColumn(TEXT("Rank"));
			SlateIM::AddTableColumn(TEXT("Fitness"));

			for (int32 i = 0; i < Total; ++i)
			{
				if (SlateIM::NextTableCell()) SlateIM::Text(FString::FromInt(i));
				if (SlateIM::NextTableCell()) SlateIM::Text(FString::SanitizeFloat(CachedAllSolutionsFitness[i]));
			}
			SlateIM::EndTable();
		}
	}
	SlateIM::EndScrollBox();

	SlateIM::EndRoot();
}
