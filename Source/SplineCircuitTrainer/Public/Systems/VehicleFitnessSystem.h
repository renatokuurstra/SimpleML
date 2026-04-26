//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleFitnessSystem.generated.h"

/**
 * UVehicleFitnessSystem
 * Assigns fitness to entities based on segment-based progress tracking.
 * Uses a cumulative exponential formula: EffectiveSegment = LapsCompleted * NumSegments + MaxSegmentReached (capped at 30).
 * Fitness = 2^EffectiveSegment + NormalizedDistanceInSegment.
 * This ensures fitness keeps growing exponentially as the car completes more laps, instead of plateauing after the first lap.
 * Backward movement reduces fitness by resetting MaxSegmentReached to the current segment.
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
