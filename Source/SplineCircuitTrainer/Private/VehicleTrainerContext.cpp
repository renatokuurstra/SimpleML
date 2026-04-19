//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "VehicleTrainerContext.h"
#include "Components/SplineComponent.h"
#include "VehicleTrainerConfig.h"
#include "Systems/SimpleMLNNFloatFeedforwardSystem.h"
#include "Systems/SimpleMLNNFloatInitSystem.h"
#include "Systems/VehicleEntityFactory.h"
#include "Systems/VehicleNNOutputSystem.h"
#include "Systems/VehicleNNInputSystem.h"
#include "Systems/VehicleProgressSystem.h"
#include "Systems/VehicleResetFlagSystem.h"
#include "Systems/VehicleFitnessSystem.h"
#include "Systems/VehicleFitnessEligibilitySystem.h"
#include "Systems/EliteSelectionFloatSystem.h"
#include "Systems/TournamentSelectionSystem.h"
#include "Systems/BreedFloatGenomesSystem.h"
#include "Systems/MutationFloatGenomeSystem.h"
#include "Systems/VehicleResetSystem.h"
#include "Systems/GAStalenessSystem.h"
#include "Systems/GACleanupSystem.h"
#include "Systems/GADebugDataSystem.h"
#include "Systems/VehicleTrainerDebugSystem.h"
#include "Components/GenomeComponents.h"

#include "Blueprint/UserWidget.h"
#include "UI/VehicleTrainerDebugWidget.h"
#include "GameFramework/PlayerController.h"

FName EvaluateNetworkEvent = FName("EvaluateNetworks");
FName GAEvaluationEvent = FName("GAEvaluationEvent");

AVehicleTrainerContext::AVehicleTrainerContext()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Create a dedicated debug entity
	const entt::entity DebugEntity = GetRegistry().create();
	GetRegistry().emplace<FGeneticAlgorithmDebugComponent>(DebugEntity);
	
	auto& BeginPlayEvent = EcsChainEvents.ChainEvents.FindOrAdd(FEcsChainEventNames::BeginPlay);

	BeginPlayEvent.Elements.Add(CreateDefaultSubobject<UVehicleEntityFactory>("VehicleEntityFactory"));
	BeginPlayEvent.Elements.Add(CreateDefaultSubobject<USimpleMLNNFloatInitSystem>("NNFloatInitSys"));
	
	auto& EvaluateEvent = EcsChainEvents.ChainEvents.FindOrAdd(EvaluateNetworkEvent);
	
	EvaluateEvent.Elements.Add(CreateDefaultSubobject<USimpleMLNNFloatFeedforwardSystem>("FeedForwardSys"));
	EvaluateEvent.Elements.Add(CreateDefaultSubobject<UVehicleNNInputSystem>("NNInputSys"));
	EvaluateEvent.Elements.Add(CreateDefaultSubobject<UVehicleNNOutputSystem>("NNOutputSys"));
	EvaluateEvent.bIsUpdateSystems = true;
	EvaluateEvent.UpdateFreqSec = 0.1f;
	
	auto& GAEvent = EcsChainEvents.ChainEvents.FindOrAdd(GAEvaluationEvent);
	
	GAEvent.Elements.Add(CreateDefaultSubobject<UVehicleProgressSystem>("ProgressSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UVehicleResetFlagSystem>("ResetFlagSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UVehicleFitnessEligibilitySystem>("FitnessEligibilitySys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UVehicleFitnessSystem>("FitnessSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UGAStalenessSystem>("StalenessSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UEliteSelectionFloatSystem>("EliteSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UTournamentSelectionSystem>("SelectionSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UBreedFloatGenomesSystem>("BreedSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UMutationFloatGenomeSystem>("MutationSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UVehicleResetSystem>("ResetSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UGACleanupSystem>("CleanupSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UGADebugDataSystem>("GADebugDataSys"));
	GAEvent.Elements.Add(CreateDefaultSubobject<UVehicleTrainerDebugSystem>("TrainerDebugSys"));
	
	GAEvent.bIsUpdateSystems = true;
	GAEvent.UpdateFreqSec = 0.5f;

	InitializeSystemsFromConfig();
}

void AVehicleTrainerContext::PostInitProperties()
{
	Super::PostInitProperties();
	InitializeSystemsFromConfig();
}

void AVehicleTrainerContext::PostLoad()
{
	Super::PostLoad();
	InitializeSystemsFromConfig();
}

void AVehicleTrainerContext::InitializeSystemsFromConfig()
{
	if (!TrainerConfig)
	{
		return;
	}

	// Update Chain Event frequencies
	if (FChainEventData* EvaluateEvent = EcsChainEvents.ChainEvents.Find(EvaluateNetworkEvent))
	{
		EvaluateEvent->UpdateFreqSec = TrainerConfig->NetworkUpdateFrequencyMS / 1000.0f;
	}

	// Iterate through systems in NewGenerationEvent and EvaluateNetworkEvent to initialize parameters
	TArray<FName> EventNames = { EvaluateNetworkEvent, GAEvaluationEvent, FEcsChainEventNames::BeginPlay };
	for (const FName& EventName : EventNames)
	{
		if (FChainEventData* EventData = EcsChainEvents.ChainEvents.Find(EventName))
		{
			for (auto& Element : EventData->Elements)
			{
				if (UEliteSelectionFloatSystem* EliteSys = Cast<UEliteSelectionFloatSystem>(Element.GetInterface()))
				{
					EliteSys->EliteCount = TrainerConfig->EliteCount;
					EliteSys->bHigherIsBetter = TrainerConfig->bHigherIsBetter;
				}
				else if (UTournamentSelectionSystem* SelectionSys = Cast<UTournamentSelectionSystem>(Element.GetInterface()))
				{
					SelectionSys->TournamentSize = TrainerConfig->TournamentSize;
					SelectionSys->SelectionPressure = TrainerConfig->SelectionPressure;
					SelectionSys->bHigherIsBetter = TrainerConfig->bHigherIsBetter;
					SelectionSys->ContextSeed = RandomSeed;
				}
				else if (UBreedFloatGenomesSystem* BreedSys = Cast<UBreedFloatGenomesSystem>(Element.GetInterface()))
				{
					BreedSys->Eta = TrainerConfig->BreedingEta;
				}
				else if (UMutationFloatGenomeSystem* MutationSys = Cast<UMutationFloatGenomeSystem>(Element.GetInterface()))
				{
					MutationSys->PerValueDeltaPercent = TrainerConfig->PerValueDeltaPercent;
					MutationSys->RandomMutationChance = TrainerConfig->RandomMutationChance;
					MutationSys->ContextSeed = RandomSeed;
				}
 			else if (USimpleMLNNFloatInitSystem* InitSys = Cast<USimpleMLNNFloatInitSystem>(Element.GetInterface()))
				{
					InitSys->ContextSeed = RandomSeed;
				}
				else if (UGAStalenessSystem* StalenessSys = Cast<UGAStalenessSystem>(Element.GetInterface()))
				{
					// Staleness system uses config directly from context
				}
				else if (UVehicleEntityFactory* FactorySys = Cast<UVehicleEntityFactory>(Element.GetInterface()))
				{
					FactorySys->ContextIndex = ContextIndex;
				}
			}
		}
	}
}

void AVehicleTrainerContext::BeginPlay()
{
	InitializeSystemsFromConfig();
	Super::BeginPlay();

	// Pre-warm nuke cooldown so no population can be nuked immediately on startup
	if (TrainerConfig)
	{
		const float NowTime = GetWorld()->GetTimeSeconds();
		for (int32 i = 0; i < TrainerConfig->NumPopulations; ++i)
		{
			NextNukeAvailableTimePerPopulation.FindOrAdd(i) = NowTime + TrainerConfig->StalenessCooldown;
		}
	}

	// Bind H key for toggling debug UI
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		EnableInput(PC);
		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AVehicleTrainerContext::ToggleDebugUI);
		}
	}
}

void AVehicleTrainerContext::ToggleDebugUI()
{
	if (!DebugWidget)
	{
		if (DebugWidgetClass)
		{
			DebugWidget = CreateWidget<UVehicleTrainerDebugWidget>(GetWorld(), DebugWidgetClass);
			if (DebugWidget)
			{
				DebugWidget->AddToViewport();
				
				// Initialize the widget with this context if needed
				// For now just assume it finds the context it needs or we set it later
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

void AVehicleTrainerContext::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(NetworkUpdateTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AVehicleTrainerContext::OnEvaluateNetworks()
{
	ExecuteEvent(EvaluateNetworkEvent);
}

USplineComponent* AVehicleTrainerContext::GetCircuitSpline() const
{
	if (!CircuitActor)
	{
		return nullptr;
	}

	return CircuitActor->FindComponentByClass<USplineComponent>();
}
