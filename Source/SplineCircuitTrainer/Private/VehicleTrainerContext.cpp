//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "VehicleTrainerContext.h"
#include "Components/SplineComponent.h"
#include "VehicleTrainerConfig.h"

AVehicleTrainerContext::AVehicleTrainerContext()
{
	PrimaryActorTick.bCanEverTick = true;
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
