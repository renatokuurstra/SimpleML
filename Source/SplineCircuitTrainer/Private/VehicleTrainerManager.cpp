//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "VehicleTrainerManager.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "VehicleComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "Blueprint/UserWidget.h"
#include "UI/VehicleTrainerDebugWidget.h"
#include "Components/NetworkComponent.h"
#include "Systems/EliteSelectionBaseSystem.h"
#include "Components/NNIOComponents.h"

AVehicleTrainerManager::AVehicleTrainerManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AVehicleTrainerManager::BeginPlay()
{
	Super::BeginPlay();
	SpawnContexts();

	// Bind H key for toggling debug UI
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		EnableInput(PC);
		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AVehicleTrainerManager::ToggleDebugUI);
		}
	}

	if (TrainerConfig != nullptr)
	{
		GetWorldTimerManager().SetTimer(UpdateTimerHandle, this, &AVehicleTrainerManager::OnUpdate, TrainerConfig->DebugLogFrequency, true);
	}
}

void AVehicleTrainerManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AVehicleTrainerManager::OnUpdate()
{
	LogEliteFitness();
	UpdateDebugWidget();
	CheckForStalenessAndNuke();
}

void AVehicleTrainerManager::LogEliteFitness()
{
	if (TrainerConfig == nullptr || !TrainerConfig->bDebugInfo || Contexts.Num() == 0)
	{
		return;
	}

	FString LogString;
	for (int32 i = 0; i < Contexts.Num(); ++i)
	{
		AVehicleTrainerContext* Context = Contexts[i];
		if (Context != nullptr)
		{
			float TotalEliteFitness = 0.0f;
			entt::registry& Registry = Context->GetRegistry();
			auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent>();

			for (auto E : EliteView)
			{
				const FFitnessComponent& Fit = EliteView.get<FFitnessComponent>(E);
				if (Fit.Fitness.Num() > 0)
				{
					TotalEliteFitness += Fit.Fitness[0];
				}
			}

			LogString += FString::Printf(TEXT("C%d: %.2f "), i + 1, TotalEliteFitness);
		}
	}

	if (!LogString.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *LogString);
	}
}

void AVehicleTrainerManager::ToggleDebugUI()
{
	if (!DebugWidget)
	{
		if (DebugWidgetClass)
		{
			DebugWidget = CreateWidget<UVehicleTrainerDebugWidget>(GetWorld(), DebugWidgetClass);
			if (DebugWidget)
			{
				DebugWidget->AddToViewport();
			}
		}
	}
	else
	{
		if (DebugWidget->IsVisible())
		{
			DebugWidget->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			DebugWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void AVehicleTrainerManager::UpdateDebugWidget()
{
	if (!DebugWidget || !DebugWidget->IsVisible())
	{
		return;
	}

	TArray<FVehicleTrainerContextUIData> UIContexts;
	for (AVehicleTrainerContext* Context : Contexts)
	{
		if (!Context) continue;

		entt::registry& Registry = Context->GetRegistry();
		auto DebugView = Registry.view<FGeneticAlgorithmDebugComponent>();
		if (DebugView.begin() != DebugView.end())
		{
			const auto& DebugComp = DebugView.get<FGeneticAlgorithmDebugComponent>(*DebugView.begin());
			
			FVehicleTrainerContextUIData Data;
			Data.ContextName = Context->GetName();
			Data.HistoricalTotalEliteFitness = DebugComp.HistoricalTotalEliteFitness;
			Data.ResetCount = DebugComp.ResetCount;

			// Gather Alive Entities directly from registry
			auto PopView = Registry.view<FFitnessComponent>();
			for (auto E : PopView)
			{
				// Alive = No ResetGenomeComponent
				if (!Registry.any_of<FResetGenomeComponent>(E))
				{
					FSplineCircuitAliveEntityUIData EntityData;
					const auto& Fit = PopView.get<FFitnessComponent>(E);
					EntityData.Fitness = (Fit.Fitness.Num() > 0) ? Fit.Fitness[0] : 0.0f;

					// Get Name from VehicleComponent if it exists
					if (Registry.any_of<FVehicleComponent>(E))
					{
						const auto& Vehicle = Registry.get<FVehicleComponent>(E);
						if (Vehicle.VehiclePawn)
						{
							EntityData.Name = Vehicle.VehiclePawn->GetName();
						}
					}

					if (EntityData.Name.IsEmpty())
					{
						EntityData.Name = FString::Printf(TEXT("Entity_%d"), (uint32)E);
					}

					Data.AliveEntities.Add(EntityData);
				}
			}

			// Sort alive entities by fitness
			Data.AliveEntities.Sort([](const FSplineCircuitAliveEntityUIData& A, const FSplineCircuitAliveEntityUIData& B) {
				return A.Fitness > B.Fitness;
			});

			UIContexts.Add(Data);
		}
	}

	if (UIContexts.Num() > 0)
	{
		DebugWidget->UpdateContextData(UIContexts);
	}
}

void AVehicleTrainerManager::CheckForStalenessAndNuke()
{
	if (!TrainerConfig || !TrainerConfig->bEnableNuke || Contexts.Num() < 2)
	{
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	AVehicleTrainerContext* WorstStaleContext = nullptr;
	float WorstStaleFitness = TNumericLimits<float>::Max();

	AVehicleTrainerContext* BestOverallContext = nullptr;
	float BestOverallFitness = -TNumericLimits<float>::Max();
	entt::entity BestEliteEntity = entt::null;

	for (AVehicleTrainerContext* Context : Contexts)
	{
		if (!Context) continue;

		entt::registry& Registry = Context->GetRegistry();
		
		// Find best elite in this context
		auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent, FNeuralNetworkFloat>();
		float ContextBestFitness = -TNumericLimits<float>::Max();
		entt::entity ContextBestElite = entt::null;

		for (auto E : EliteView)
		{
			const auto& Fit = EliteView.get<FFitnessComponent>(E);
			float CurrentFitness = (Fit.Fitness.Num() > 0) ? Fit.Fitness[0] : 0.0f;
			if (CurrentFitness > ContextBestFitness)
			{
				ContextBestFitness = CurrentFitness;
				ContextBestElite = E;
			}
		}
		
		// Special case: if no elites yet, fitness is 0
		if (ContextBestFitness < 0) ContextBestFitness = 0.0f;

		UE_LOG(LogTemp, Verbose, TEXT("[AVehicleTrainerManager] Context %s best fitness: %.2f (Elite: %u)"), 
			*Context->GetName(), ContextBestFitness, (uint32)ContextBestElite);

		// Update global best
		if (ContextBestElite != entt::null && ContextBestFitness >= BestOverallFitness)
		{
			BestOverallFitness = ContextBestFitness;
			BestOverallContext = Context;
			BestEliteEntity = ContextBestElite;
		}

		// Check for staleness
		if (CurrentTime < Context->NextNukeAvailableTime)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[AVehicleTrainerManager] Context %s on cooldown. Next available in %.1fs"), *Context->GetName(), Context->NextNukeAvailableTime - CurrentTime);
			continue;
		}

		auto DebugView = Registry.view<FGeneticAlgorithmDebugComponent>();
		if (DebugView.begin() == DebugView.end()) continue;

		const auto& DebugComp = DebugView.get<FGeneticAlgorithmDebugComponent>(*DebugView.begin());
		const TArray<float>& History = DebugComp.HistoricalTotalEliteFitness;

		if (History.Num() < TrainerConfig->MinHistoryForStaleness)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[AVehicleTrainerManager] Context %s has insufficient history: %d/%d"), 
				*Context->GetName(), History.Num(), TrainerConfig->MinHistoryForStaleness);
			continue;
		}

		// Windowed Mean Comparison
		const int32 WindowSize = 10;
		float OldSum = 0.0f;
		float NewSum = 0.0f;

		for (int32 i = 0; i < WindowSize; ++i)
		{
			OldSum += History[i];
			NewSum += History[History.Num() - 1 - i];
		}

		float OldMean = OldSum / WindowSize;
		float NewMean = NewSum / WindowSize;

		float Improvement = (NewMean - OldMean) / FMath::Max(FMath::Abs(OldMean), 1e-6f);

		UE_LOG(LogTemp, Verbose, TEXT("[AVehicleTrainerManager] Context %s: MeanOld: %.2f, MeanNew: %.2f, Improvement: %.4f (Threshold: %.4f)"), 
			*Context->GetName(), OldMean, NewMean, Improvement, TrainerConfig->StalenessThreshold);

		if (Improvement < TrainerConfig->StalenessThreshold)
		{
			// This context is stale. Is it the worst?
			if (ContextBestFitness < WorstStaleFitness)
			{
				WorstStaleFitness = ContextBestFitness;
				WorstStaleContext = Context;
			}
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("[AVehicleTrainerManager] Context %s not stale. Improvement: %.4f (Threshold: %.4f)"), 
				*Context->GetName(), Improvement, TrainerConfig->StalenessThreshold);
		}
	}

	if (!WorstStaleContext)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[AVehicleTrainerManager] No stale context found."));
		return;
	}

	if (WorstStaleContext)
	{
		if (BestEliteEntity == entt::null)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AVehicleTrainerManager] WorstStaleContext %s found, but BestEliteEntity is null. Cannot nuke."), *WorstStaleContext->GetName());
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[AVehicleTrainerManager] Evaluating nuke for WorstStaleContext: %s, BestEliteEntity: %u"), 
			*WorstStaleContext->GetName(), (uint32)BestEliteEntity);
		
		// Even if it's the best overall, if it's stale and we have no other options, we might not nuke.
		// BUT if there's only one context, we can't nuke (needs pioneer).
		// If there are multiple, and the best one is stale... that's a problem.
		// The requirement was: "simulation is stale and it's the worst config".
		
		if (WorstStaleContext == BestOverallContext)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AVehicleTrainerManager] Worst stale context %s is also the best overall context. Cannot nuke without a better pioneer."), *WorstStaleContext->GetName());
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[AVehicleTrainerManager] Nuking stale context: %s. Best Fitness: %.2f. Pioneering from %s (Fitness: %.2f)"), 
			*WorstStaleContext->GetName(), WorstStaleFitness, *BestOverallContext->GetName(), BestOverallFitness);

		entt::registry& TargetRegistry = WorstStaleContext->GetRegistry();
		entt::registry& SourceRegistry = BestOverallContext->GetRegistry();

		// 1. Pioneer Injection: Replace the worst elite in target context with the best genome from source
		auto TargetEliteView = TargetRegistry.view<FEliteTagComponent, FNeuralNetworkFloat>();
		entt::entity WorstTargetElite = entt::null;
		float MinTargetEliteFitness = TNumericLimits<float>::Max();

		for (auto E : TargetEliteView)
		{
			if (TargetRegistry.any_of<FFitnessComponent>(E))
			{
				const auto& Fit = TargetRegistry.get<FFitnessComponent>(E);
				float CurrentFitness = (Fit.Fitness.Num() > 0) ? Fit.Fitness[0] : 0.0f;
				if (CurrentFitness < MinTargetEliteFitness)
				{
					MinTargetEliteFitness = CurrentFitness;
					WorstTargetElite = E;
				}
			}
			else
			{
				WorstTargetElite = E;
				break;
			}
		}

		if (WorstTargetElite != entt::null)
		{
			const auto& SourceWeights = SourceRegistry.get<FNeuralNetworkFloat>(BestEliteEntity).Network.GetData();
			TargetRegistry.get<FNeuralNetworkFloat>(WorstTargetElite).Network.GetData() = SourceWeights;
			
			// Reset fitness of the new pioneer to give it a fresh start in the new context
			if (TargetRegistry.any_of<FFitnessComponent>(WorstTargetElite))
			{
				TargetRegistry.get<FFitnessComponent>(WorstTargetElite).Fitness.Reset();
			}

			// Clear IO components to prevent using old values with new weights
			if (TargetRegistry.any_of<FNNInFLoatComp>(WorstTargetElite))
			{
				TargetRegistry.get<FNNInFLoatComp>(WorstTargetElite).Values.Reset();
			}
			if (TargetRegistry.any_of<FNNOutFloatComp>(WorstTargetElite))
			{
				TargetRegistry.get<FNNOutFloatComp>(WorstTargetElite).Values.Reset();
			}
		}

		// 2. Reset all non-elite entities
		auto AllEntities = TargetRegistry.view<entt::entity>();
		for (auto E : AllEntities)
		{
			if (!TargetRegistry.any_of<FEliteTagComponent>(E))
			{
				TargetRegistry.emplace_or_replace<FResetGenomeComponent>(E);
			}
		}

		// 3. Set Cooldown
		WorstStaleContext->NextNukeAvailableTime = CurrentTime + TrainerConfig->StalenessCooldown;
	}
}

void AVehicleTrainerManager::SpawnContexts()
{
	if (!TrainerConfig || !CircuitActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AVehicleTrainerManager] Missing TrainerConfig or CircuitActor. Cannot spawn contexts."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 i = 0; i < NumContexts; ++i)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Name = FName(*FString::Printf(TEXT("VehicleTrainerContext_%d"), i));
		
		AVehicleTrainerContext* NewContext = World->SpawnActorDeferred<AVehicleTrainerContext>(AVehicleTrainerContext::StaticClass(), FTransform(GetActorRotation(), GetActorLocation()), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (NewContext)
		{
#if WITH_EDITOR
			NewContext->SetActorLabel(FString::Printf(TEXT("VehicleTrainerContext_%d"), i));
#endif
			NewContext->TrainerConfig = TrainerConfig;
			NewContext->CircuitActor = CircuitActor;
			NewContext->ContextIndex = i;
			
			// Assign seed: use ContextSeeds if available, otherwise use index
			if (ContextSeeds.IsValidIndex(i))
			{
				NewContext->RandomSeed = ContextSeeds[i];
			}
			else
			{
				NewContext->RandomSeed = i;
			}

			NewContext->InitializeSystemsFromConfig();
			NewContext->FinishSpawning(FTransform(GetActorRotation(), GetActorLocation()));
			Contexts.Add(NewContext);
		}
	}
}
