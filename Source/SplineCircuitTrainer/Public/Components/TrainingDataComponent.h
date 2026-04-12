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
};
