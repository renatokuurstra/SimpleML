// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VehicleNNInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class USimpleMLVehicleNNInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for vehicles that can be controlled by a Neural Network.
 */
class SIMPLEMLINTERFACES_API ISimpleMLVehicleNNInterface
{
	GENERATED_BODY()

public:
	/**
	 * Apply outputs from a Neural Network to the vehicle.
	 * @param Outputs Array of float values (typically Throttle, Brake, Steering).
	 */
	UFUNCTION(BlueprintCallable, Category = "Vehicle|NN")
	virtual void ApplyNNOutputs(const TArray<float>& Outputs) = 0;
};
