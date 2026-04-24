//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleFitnessSystem.generated.h"

/**
 * UVehicleFitnessSystem
 * Assigns fitness to entities based on segment-based progress tracking.
 * Uses an exponential formula: Fitness = 2^MaxSegmentReached + NormalizedDistanceInSegment (+ lap bonus for closed loops).
 * This prevents backward movement from increasing fitness since MaxSegmentReached is monotonic.
 * Fitness is always recalculated, never accumulated/stacked.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleFitnessSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleFitnessSystem();

	virtual void Update_Implementation(float DeltaTime) override;
};
