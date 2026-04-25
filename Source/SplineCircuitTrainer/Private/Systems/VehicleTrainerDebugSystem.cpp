//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleTrainerDebugSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "SlateIM.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "VehicleComponent.h"
#include "GameFramework/Pawn.h"

UVehicleTrainerDebugSystem::UVehicleTrainerDebugSystem()
{
	RegisterComponent<FGeneticAlgorithmDebugComponent>();
	RegisterComponent<FElitePromotionDebugComponent>();
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

	// Compute per-population elite fitness directly from elite entities
	// (more reliable than relying on PopulationTotalEliteFitness map)
	// Throttled to once per second to avoid log spam
	{
		EliteFitnessLogTimer += DeltaTime;
		if (EliteFitnessLogTimer < EliteFitnessLogInterval)
		{
			// Still update cached data every tick, but skip logging
			entt::registry& Registry = GetRegistry();
			auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent>();

			TMap<int32, float> PerPopEliteFitness;
			TMap<int32, int32> PerPopEliteCount;

			for (auto E : EliteView)
			{
				const FFitnessComponent& Fit = EliteView.get<FFitnessComponent>(E);
				const int32 PopIdx = Fit.BuiltForFitnessIndex;
				if (PopIdx >= 0 && Fit.Fitness.IsValidIndex(PopIdx))
				{
					// Skip neutral fitness values (not yet evaluated)
					const float Val = Fit.Fitness[PopIdx];
					const float NeutralVal = TrainerContext->TrainerConfig->bHigherIsBetter ? -MAX_FLT : MAX_FLT;
					if (Val != NeutralVal)
					{
						PerPopEliteFitness.FindOrAdd(PopIdx) += Val;
						PerPopEliteCount.FindOrAdd(PopIdx)++;
					}
				}
			}

			// Update the debug component's map so other consumers get the data
			if (PerPopEliteFitness.Num() > 0)
			{
				FGeneticAlgorithmDebugComponent& MutableDebugComp = const_cast<FGeneticAlgorithmDebugComponent&>(DebugComp);
				MutableDebugComp.PopulationTotalEliteFitness = PerPopEliteFitness;
			}
		}
		else
		{
			EliteFitnessLogTimer = 0.0f;

			entt::registry& Registry = GetRegistry();
			auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent>();

			// Group elites by BuiltForFitnessIndex and sum their fitness
			TMap<int32, float> PerPopEliteFitness;
			TMap<int32, int32> PerPopEliteCount;

			for (auto E : EliteView)
			{
				const FFitnessComponent& Fit = EliteView.get<FFitnessComponent>(E);
				const int32 PopIdx = Fit.BuiltForFitnessIndex;
				if (PopIdx >= 0 && Fit.Fitness.IsValidIndex(PopIdx))
				{
					// Skip neutral fitness values (not yet evaluated)
					const float Val = Fit.Fitness[PopIdx];
					const float NeutralVal = TrainerContext->TrainerConfig->bHigherIsBetter ? -MAX_FLT : MAX_FLT;
					if (Val != NeutralVal)
					{
						PerPopEliteFitness.FindOrAdd(PopIdx) += Val;
						PerPopEliteCount.FindOrAdd(PopIdx)++;
					}
				}
			}

			if (PerPopEliteFitness.Num() > 0)
			{
				FString LogStr = FString::Printf(TEXT("[ELITE FITNESS]"));
				float OverallTotal = 0.0f;

				TArray<int32> PopIndices;
				PerPopEliteFitness.GetKeys(PopIndices);
				PopIndices.Sort();

				for (int32 PopIdx : PopIndices)
				{
					const float Val = PerPopEliteFitness[PopIdx];
					const int32 Count = PerPopEliteCount.FindRef(PopIdx);
					LogStr += FString::Printf(TEXT(" Pop%d(cnt=%d): %.2f |"), PopIdx, Count, Val);
					OverallTotal += Val;
				}
				LogStr += FString::Printf(TEXT(" TOTAL: %.2f"), OverallTotal);

				UE_LOG(LogTemp, Log, TEXT("%s"), *LogStr);

				// Update historical data
				FGeneticAlgorithmDebugComponent& MutableDebugComp = const_cast<FGeneticAlgorithmDebugComponent&>(DebugComp);
				MutableDebugComp.HistoricalTotalEliteFitness.Add(OverallTotal);
				if (MutableDebugComp.HistoricalTotalEliteFitness.Num() > MutableDebugComp.MaxHistoryLength)
				{
					MutableDebugComp.HistoricalTotalEliteFitness.RemoveAt(0);
				}

				// Also update the debug component's map so other consumers get the data
				MutableDebugComp.PopulationTotalEliteFitness = PerPopEliteFitness;
			}
		}
	}

	// Draw Elite Promotion Debug
	float WorldTime = GetContext()->GetWorld()->GetTimeSeconds();
	auto ElitePromoView = GetView<FElitePromotionDebugComponent>();
	entt::registry& Registry = GetRegistry();

	for (auto Entity : ElitePromoView)
	{
		FElitePromotionDebugComponent& PromoComp = ElitePromoView.get<FElitePromotionDebugComponent>(Entity);
		if (WorldTime > PromoComp.ExpirationTime)
		{
			Registry.remove<FElitePromotionDebugComponent>(Entity);
			continue;
		}

		// Use the stored location snapshot (not dynamically looked up)
		// This prevents dots from following reset vehicles to the spawn point
		if (PromoComp.Location != FVector::ZeroVector)
		{
			DrawDebugPoint(GetContext()->GetWorld(), PromoComp.Location, 20.0f, FColor::Black, false, 20.0f, 0);
		}
	}
}

void UVehicleTrainerDebugSystem::Deinitialize_Implementation()
{
	Super::Deinitialize_Implementation();
}

void UVehicleTrainerDebugSystem::DrawDebugUI()
{
}
