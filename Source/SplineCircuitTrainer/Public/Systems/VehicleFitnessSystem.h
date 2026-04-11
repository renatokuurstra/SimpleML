//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleFitnessSystem.generated.h"

/**
 * UVehicleFitnessSystem
 * Assigns fitness to entities based on their training data.
 * Specifically, it cubes the DistanceTraveled to strongly favor distance in selection.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleFitnessSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleFitnessSystem();

	virtual void Update_Implementation(float DeltaTime) override;
};
