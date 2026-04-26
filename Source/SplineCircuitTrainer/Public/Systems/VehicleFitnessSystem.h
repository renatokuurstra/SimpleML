//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleFitnessSystem.generated.h"

/**
 * UVehicleFitnessSystem
 * Assigns fitness to entities based on segment-based progress tracking.
 * Uses Sum(SegmentPassCount) as the cumulative measure of total progress across all laps.
 * Formula: EffectiveSegment = Sum(SegmentPassCount) capped at 30.
 * Fitness = 2^EffectiveSegment + NormalizedDistanceInSegment.
 * SegmentPassCount is incremented on every confirmed forward segment transition, so the sum
 * naturally grows by NumSegments per lap — no separate lap counter needed.
 * Backward movement triggers a reset (SegmentPassCount zeroed), so backward movers get low fitness.
 * Fitness is always recalculated from scratch, never accumulated/stacked.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleFitnessSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleFitnessSystem();

	virtual void Update_Implementation(float DeltaTime) override;
};
