//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "VehicleComponent.generated.h"

class APawn;

/**
 * FVehicleComponent
 * Holds a reference to the pawn associated with an ECS entity.
 */
USTRUCT(BlueprintType)
struct SPLINECIRCUITTRAINER_API FVehicleComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	TObjectPtr<APawn> VehiclePawn;
};
