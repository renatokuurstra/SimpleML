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
#include "Systems/GACleanupSystem.h"
#include "Systems/GADebugDataSystem.h"
#include "Systems/VehicleTrainerDebugSystem.h"
#include "Components/GenomeComponents.h"

FName EvaluateNetworkEvent = FName("EvaluateNetworks");
FName NewGenerationEvent = FName("NewGeneration");

AVehicleTrainerContext::AVehicleTrainerContext()
{
	PrimaryActorTick.bCanEverTick = true;
	
	auto& BeginPlayEvent = EcsChainEvents.ChainEvents.FindOrAdd(FEcsChainEventNames::BeginPlay);

	//Create all necessary entities with proper components
	UVehicleEntityFactory* VehicleEntityFactory = CreateDefaultSubobject<UVehicleEntityFactory>("VehicleEntityFactory");
	//Initialise all NN on entities
	USimpleMLNNFloatInitSystem* InitSystem = CreateDefaultSubobject<USimpleMLNNFloatInitSystem>("NNFloatInitSys");

	BeginPlayEvent.Elements.Add(VehicleEntityFactory);
	BeginPlayEvent.Elements.Add(InitSystem);

	// Create a dedicated debug entity
	const entt::entity DebugEntity = GetRegistry().create();
	GetRegistry().emplace<FGeneticAlgorithmDebugComponent>(DebugEntity);
	
	auto& EvaluateEvent = EcsChainEvents.ChainEvents.FindOrAdd(EvaluateNetworkEvent);
	
	//Feedforward, move inputs to outputs
	USimpleMLNNFloatFeedforwardSystem* NNFeedForward = CreateDefaultSubobject<USimpleMLNNFloatFeedforwardSystem>("FeedForwardSys");
	//Prepare neural network inputs
	UVehicleNNInputSystem* NNInputSystem = CreateDefaultSubobject<UVehicleNNInputSystem>("NNInputSys");
	//Move outputs of networks to the pawn movement functions
	UVehicleNNOutputSystem* NNOutputSystem = CreateDefaultSubobject<UVehicleNNOutputSystem>("NNOutputSys");
	
	EvaluateEvent.Elements.Add(NNInputSystem);
	EvaluateEvent.Elements.Add(NNFeedForward);
	EvaluateEvent.Elements.Add(NNOutputSystem);
	
	EvaluateEvent.bIsUpdateSystems = true;
	EvaluateEvent.UpdateFreqSec = 0.1f;
	
	auto& GAEvent = EcsChainEvents.ChainEvents.FindOrAdd(NewGenerationEvent);
	
	UVehicleProgressSystem* ProgressSystem = CreateDefaultSubobject<UVehicleProgressSystem>("ProgressSys");
	UVehicleResetFlagSystem* ResetFlagSystem = CreateDefaultSubobject<UVehicleResetFlagSystem>("ResetFlagSys");
	UVehicleFitnessEligibilitySystem* FitnessEligibilitySystem = CreateDefaultSubobject<UVehicleFitnessEligibilitySystem>("FitnessEligibilitySys");
	UVehicleFitnessSystem* FitnessSystem = CreateDefaultSubobject<UVehicleFitnessSystem>("FitnessSys");
	UEliteSelectionFloatSystem* EliteSys = CreateDefaultSubobject<UEliteSelectionFloatSystem>("EliteSys");
	UTournamentSelectionSystem* SelectionSys = CreateDefaultSubobject<UTournamentSelectionSystem>("SelectionSys");
	UBreedFloatGenomesSystem* BreedSys = CreateDefaultSubobject<UBreedFloatGenomesSystem>("BreedSys");
	UMutationFloatGenomeSystem* MutationSys = CreateDefaultSubobject<UMutationFloatGenomeSystem>("MutationSys");
	UVehicleResetSystem* ResetSystem = CreateDefaultSubobject<UVehicleResetSystem>("ResetSys");
	UGACleanupSystem* CleanupSys = CreateDefaultSubobject<UGACleanupSystem>("CleanupSys");

	
	GAEvent.Elements.Add(ProgressSystem);
	GAEvent.Elements.Add(ResetFlagSystem);
	GAEvent.Elements.Add(FitnessEligibilitySystem);
	GAEvent.Elements.Add(FitnessSystem);
	GAEvent.Elements.Add(EliteSys);
	GAEvent.Elements.Add(SelectionSys);
	GAEvent.Elements.Add(BreedSys);
	GAEvent.Elements.Add(MutationSys);
	GAEvent.Elements.Add(ResetSystem);
	GAEvent.Elements.Add(CleanupSys);

	UGADebugDataSystem* GADebugDataSys = CreateDefaultSubobject<UGADebugDataSystem>("GADebugDataSys");
	UVehicleTrainerDebugSystem* VehicleDebugSys = CreateDefaultSubobject<UVehicleTrainerDebugSystem>("VehicleDebugSys");
	
	GAEvent.Elements.Add(GADebugDataSys);
	GAEvent.Elements.Add(VehicleDebugSys);
	
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
	TArray<FName> EventNames = { EvaluateNetworkEvent, NewGenerationEvent, FEcsChainEventNames::BeginPlay };
	for (const FName& EventName : EventNames)
	{
		if (FChainEventData* EventData = EcsChainEvents.ChainEvents.Find(EventName))
		{
			for (auto& Element : EventData->Elements)
			{
				if (UEliteSelectionFloatSystem* EliteSys = Cast<UEliteSelectionFloatSystem>(Element.GetInterface()))
				{
					EliteSys->EliteCount = TrainerConfig->EliteCount;
					EliteSys->bHigherIsBetter = true;
				}
				else if (UTournamentSelectionSystem* SelectionSys = Cast<UTournamentSelectionSystem>(Element.GetInterface()))
				{
					SelectionSys->TournamentSize = TrainerConfig->TournamentSize;
					SelectionSys->SelectionPressure = TrainerConfig->SelectionPressure;
					SelectionSys->bHigherIsBetter = true;
				}
				else if (UBreedFloatGenomesSystem* BreedSys = Cast<UBreedFloatGenomesSystem>(Element.GetInterface()))
				{
					BreedSys->Eta = TrainerConfig->BreedingEta;
				}
				else if (UMutationFloatGenomeSystem* MutationSys = Cast<UMutationFloatGenomeSystem>(Element.GetInterface()))
				{
					MutationSys->PerValueDeltaPercent = TrainerConfig->PerValueDeltaPercent;
					MutationSys->RandomMutationChance = TrainerConfig->RandomMutationChance;
				}
			}
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
