//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsContext.h"
#include "VehicleTrainerContext.generated.h"

class USplineComponent;
class UVehicleTrainerConfig;

/**
 * AVehicleTrainerContext
 * ECS Context for training vehicles on a spline circuit.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API AVehicleTrainerContext : public AEcsContext
{
	GENERATED_BODY()

public:
	AVehicleTrainerContext();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TObjectPtr<UVehicleTrainerConfig> TrainerConfig;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TObjectPtr<USplineComponent> CircuitSpline;
};
