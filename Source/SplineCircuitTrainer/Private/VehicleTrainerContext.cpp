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
#include "Systems/EliteSelectionFloatSystem.h"
#include "Systems/TournamentSelectionSystem.h"
#include "Systems/BreedFloatGenomesSystem.h"
#include "Systems/MutationFloatGenomeSystem.h"
#include "Systems/VehicleResetSystem.h"
#include "Systems/GACleanupSystem.h"

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
	
	auto& NewGenEvent = EcsChainEvents.ChainEvents.FindOrAdd(NewGenerationEvent);
	
	UVehicleProgressSystem* ProgressSystem = CreateDefaultSubobject<UVehicleProgressSystem>("ProgressSys");
	UVehicleResetFlagSystem* ResetFlagSystem = CreateDefaultSubobject<UVehicleResetFlagSystem>("ResetFlagSys");
	UVehicleFitnessSystem* FitnessSystem = CreateDefaultSubobject<UVehicleFitnessSystem>("FitnessSys");
	UEliteSelectionFloatSystem* EliteSys = CreateDefaultSubobject<UEliteSelectionFloatSystem>("EliteSys");
	UTournamentSelectionSystem* SelectionSys = CreateDefaultSubobject<UTournamentSelectionSystem>("SelectionSys");
	UBreedFloatGenomesSystem* BreedSys = CreateDefaultSubobject<UBreedFloatGenomesSystem>("BreedSys");
	UMutationFloatGenomeSystem* MutationSys = CreateDefaultSubobject<UMutationFloatGenomeSystem>("MutationSys");
	UVehicleResetSystem* ResetSystem = CreateDefaultSubobject<UVehicleResetSystem>("ResetSys");
	UGACleanupSystem* CleanupSys = CreateDefaultSubobject<UGACleanupSystem>("CleanupSys");

	
	NewGenEvent.Elements.Add(ProgressSystem);
	NewGenEvent.Elements.Add(ResetFlagSystem);
	NewGenEvent.Elements.Add(FitnessSystem);
	NewGenEvent.Elements.Add(EliteSys);
	NewGenEvent.Elements.Add(SelectionSys);
	NewGenEvent.Elements.Add(BreedSys);
	NewGenEvent.Elements.Add(MutationSys);
	NewGenEvent.Elements.Add(ResetSystem);
	NewGenEvent.Elements.Add(CleanupSys);
	
	NewGenEvent.bIsUpdateSystems = true;
	NewGenEvent.UpdateFreqSec = 0.5f;
}

void AVehicleTrainerContext::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(NetworkUpdateTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AVehicleTrainerContext::OnEvaluateNetworks()
{
	ExecuteEvent(TEXT("EvaluateNetworks"));
}

void AVehicleTrainerContext::NextGeneration()
{
	if (!TrainerConfig)
	{
		return;
	}

	FChainEventData* GenEventData = EcsChainEvents.ChainEvents.Find(NewGenerationEvent);
	if (!GenEventData)
	{
		return;
	}

	for (auto& Element : GenEventData->Elements)
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

	ExecuteEvent(NewGenerationEvent);
}

USplineComponent* AVehicleTrainerContext::GetCircuitSpline() const
{
	if (!CircuitActor)
	{
		return nullptr;
	}

	return CircuitActor->FindComponentByClass<USplineComponent>();
}
