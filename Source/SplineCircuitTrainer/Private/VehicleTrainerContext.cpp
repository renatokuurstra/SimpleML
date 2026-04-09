//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "VehicleTrainerContext.h"
#include "Components/SplineComponent.h"
#include "VehicleTrainerConfig.h"
#include "Systems/SimpleMLNNFloatFeedforwardSystem.h"
#include "Systems/SimpleMLNNFloatInitSystem.h"
#include "Systems/VehicleEntityFactory.h"
#include "Systems/VehicleNNOutputSystem.h"

FName EvaluateNetworkEvent = FName("EvaluateNetworks");

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
	//Move outputs of networks to the pawn movement functions
	UVehicleNNOutputSystem* NNOutputSystem = CreateDefaultSubobject<UVehicleNNOutputSystem>("NNOutputSys");
	
	EvaluateEvent.Elements.Add(NNFeedForward);
	EvaluateEvent.Elements.Add(NNOutputSystem) ;
}

void AVehicleTrainerContext::BeginPlay()
{
	Super::BeginPlay();

	if (TrainerConfig)
	{
		float Freq = TrainerConfig->NetworkUpdateFrequencyMS;
		if (Freq > 0.0f)
		{
			float UpdateRateSeconds = Freq / 1000.0f;
			GetWorldTimerManager().SetTimer(NetworkUpdateTimerHandle, this, &AVehicleTrainerContext::OnEvaluateNetworks, UpdateRateSeconds, true);
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
	ExecuteEvent(TEXT("EvaluateNetworks"));
}

USplineComponent* AVehicleTrainerContext::GetCircuitSpline() const
{
	if (!CircuitActor)
	{
		return nullptr;
	}

	return CircuitActor->FindComponentByClass<USplineComponent>();
}
