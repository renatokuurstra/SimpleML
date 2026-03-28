//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsContext.h"
#include "VehicleTrainer.generated.h"

class USplineComponent;
class APawn;

/**
 * AVehicleTrainer
 * ECS Context for training vehicles on a spline circuit.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API AVehicleTrainer : public AEcsContext
{
	GENERATED_BODY()

public:
	AVehicleTrainer();

protected:
	UPROPERTY(EditAnywhere, Category = "Trainer")
	TObjectPtr<USplineComponent> CircuitSpline;

	UPROPERTY(EditAnywhere, Category = "Trainer")
	TObjectPtr<APawn> VehiclePawn;
};
