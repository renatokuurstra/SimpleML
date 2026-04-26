//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VehicleLibrary.generated.h"

/**
 * Utility library for vehicle-related functions.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Reason for resetting a vehicle because it moved too far from the spline. */
	static const FName ReasonTooFarFromSpline;
	
	/** Reason for resetting a vehicle because it made no progress for too long. */
	static const FName ReasonNoProgress;
	
	/** Reason for resetting a vehicle because it was moving too slowly. */
	static const FName ReasonTooSlow;
	
	/** Reason for resetting a vehicle because it is not progressing correctly along the spline. */
	static const FName ReasonIncorrectProgress;

	/** Reason for resetting a vehicle because it went backward from the very start (segment 0 -> segment N-1). Triggers full genome re-randomization. */
	static const FName ReasonBackwardStart;

	/**
	 * Gets the spawn transform for a vehicle based on a spline.
	 */
	static void GetVehicleSpawnTransform(const class USplineComponent* Spline, float Distance, float VerticalOffset, FVector& OutLocation, FRotator& OutRotation);
	
	/**
	 * Resets the physical state of a pawn (location, rotation, velocity).
	 */
	static void ResetPawnPhysicalState(APawn* Pawn, const FVector& Location, const FRotator& Rotation);
	
	/**
	 * Sets up training data for a vehicle.
	 */
	static void SetTrainingData(FTrainingDataComponent& OutTrainingData, const USplineComponent* Spline, const FVector& Location, float CreationTime);
};
