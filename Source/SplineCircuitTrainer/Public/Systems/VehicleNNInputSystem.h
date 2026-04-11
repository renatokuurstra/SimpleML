//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleTrainerConfig.h"
#include "VehicleNNInputSystem.generated.h"

/**
 * UVehicleNNInputSystem
 * System that generates input values for the neural network of each vehicle in the population.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleNNInputSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleNNInputSystem();

	virtual void Update_Implementation(float DeltaTime) override;

private:
	bool bHasLoggedInputMismatch = false;
};
