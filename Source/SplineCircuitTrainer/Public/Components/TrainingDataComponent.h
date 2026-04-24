//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "TrainingDataComponent.generated.h"

/**
 * FTrainingDataComponent
 * Tracks the training progress of an entity.
 * This is used for evaluating fitness based on travel distance along a spline.
 */
USTRUCT(BlueprintType)
struct FTrainingDataComponent
{
	GENERATED_BODY()

	/** Total distance traveled along the spline, in spline coordinates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	float DistanceTraveled = 0.0f;

	/** Last recorded distance along the spline to calculate delta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	float LastSplineDistance = 0.0f;

	/** Last recorded spline segment index. Used for determining movement direction across loop start/end. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	int32 LastSplineSegment = 0;

	/** Last distance delta recorded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	float LastDistanceDelta = 0.0f;

	/** Maximum distance traveled in the current session. Used for progress tracking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	float MaxDistanceTraveled = 0.0f;

	/** Time elapsed since the last positive progress along the spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	float TimeSinceLastProgress = 0.0f;

	/** Unreal engine time since start when the entity was created. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	float CreationTime = 0.0f;

	/** Array to track how many times a car has passed through each segment of the spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	TArray<int32> SegmentPassCount;

	/** The highest segment index the vehicle has ever reached through forward movement. Used for fitness calculation to prevent backward movement from increasing fitness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	int32 MaxSegmentReached = 0;

	/** Number of complete laps completed on a closed-loop spline. Only increments on valid forward wraparound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	int32 LapsCompleted = 0;

	/** Normalized distance (0.0-1.0) within the current segment. Used for fine-grained fitness calculation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	float NormalizedDistanceInSegment = 0.0f;

	/**
	 * Resets all training data fields to initial state.
	 * Called when a vehicle is reset to its starting position.
	 */
	void Reset()
	{
		DistanceTraveled = 0.0f;
		LastSplineDistance = 0.0f;
		LastSplineSegment = 0;
		LastDistanceDelta = 0.0f;
		MaxDistanceTraveled = 0.0f;
		TimeSinceLastProgress = 0.0f;
		SegmentPassCount.Init(0, SegmentPassCount.Num());
		MaxSegmentReached = 0;
		LapsCompleted = 0;
		NormalizedDistanceInSegment = 0.0f;
	}
};
