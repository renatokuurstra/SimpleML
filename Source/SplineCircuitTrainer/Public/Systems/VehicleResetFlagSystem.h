//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleResetFlagSystem.generated.h"

/**
 * UVehicleResetFlagSystem
 * Flags entities with FResetGenomeComponent if they deviate too far from the spline
 * or fail to make progress within a certain time frame.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleResetFlagSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleResetFlagSystem();

	virtual void Update_Implementation(float DeltaTime) override;
};
