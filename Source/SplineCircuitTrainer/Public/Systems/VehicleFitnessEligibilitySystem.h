//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleFitnessEligibilitySystem.generated.h"

/**
 * UVehicleFitnessEligibilitySystem
 * Manages adding FFitnessComponent to entities based on their Age.
 * Entities only become eligible for breeding/fitness after MinBreedAge 
 * and when they are within a factor of the oldest currently alive solution.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleFitnessEligibilitySystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleFitnessEligibilitySystem();

	virtual void Update_Implementation(float DeltaTime) override;
};
