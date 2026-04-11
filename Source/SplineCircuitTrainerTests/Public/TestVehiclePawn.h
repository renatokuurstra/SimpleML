//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VehicleNNInterface.h"
#include "TestVehiclePawn.generated.h"

/**
 * A simple pawn for testing the SplineCircuitTrainer.
 * Implements ISimpleMLVehicleNNInterface as required by the trainer's output systems.
 */
UCLASS()
class ATestVehiclePawn : public APawn, public ISimpleMLVehicleNNInterface
{
	GENERATED_BODY()

public:
	// Begin ISimpleMLVehicleNNInterface
	virtual void ApplyNNOutputs(TArrayView<const float> Outputs) override
	{
		LastOutputs = TArray<float>(Outputs);
	}
	// End ISimpleMLVehicleNNInterface

	UPROPERTY()
	TArray<float> LastOutputs;
};
