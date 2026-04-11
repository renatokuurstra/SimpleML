//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/NoExportTypes.h"
#include "VehicleLibrary.generated.h"

class USplineComponent;
class AVehicleTrainerContext;

/**
 * UVehicleLibrary
 * Static utility functions for vehicle handling in the trainer.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Calculates the spawn transform for a vehicle at a given spline distance.
	 */
	static void GetVehicleSpawnTransform(const USplineComponent* Spline, float Distance, float VerticalOffset, FVector& OutLocation, FRotator& OutRotation);

	/**
	 * Resets a pawn to a specific location and rotation, clearing physical states.
	 */
	static void ResetPawnPhysicalState(APawn* Pawn, const FVector& Location, const FRotator& Rotation);
};
