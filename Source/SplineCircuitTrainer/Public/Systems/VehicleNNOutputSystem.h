//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleNNOutputSystem.generated.h"

/**
 * UVehicleNNOutputSystem
 * System that applies neural network outputs to vehicles implementing ISimpleMLVehicleNNInterface.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class SPLINECIRCUITTRAINER_API UVehicleNNOutputSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleNNOutputSystem();

	virtual void Update_Implementation(float DeltaTime) override;
};
