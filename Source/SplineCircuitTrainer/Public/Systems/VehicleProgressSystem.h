//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleProgressSystem.generated.h"

/**
 * UVehicleProgressSystem
 * Evaluates the progress of vehicle pawns along a spline.
 * Updates FTrainingDataComponent for each vehicle.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleProgressSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleProgressSystem();

	virtual void Update_Implementation(float DeltaTime) override;
};
