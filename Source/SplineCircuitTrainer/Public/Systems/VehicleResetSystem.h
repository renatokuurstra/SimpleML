//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleResetSystem.generated.h"

/**
 * UVehicleResetSystem
 * Resets the physical state of entities flagged with FResetGenomeComponent.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleResetSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleResetSystem();

	virtual void Update_Implementation(float DeltaTime) override;
};
