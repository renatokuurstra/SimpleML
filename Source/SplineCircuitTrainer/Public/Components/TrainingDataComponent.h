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
};
