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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TObjectPtr<AActor> CircuitActor;

	/** Helper function to retrieve the spline component from the CircuitActor */
	UFUNCTION(BlueprintCallable, Category = "Trainer")
	USplineComponent* GetCircuitSpline() const;

	/** Triggers the GA evolution step (Selection -> Breeding -> Mutation -> Cleanup) */
	UFUNCTION(BlueprintCallable, Category = "Trainer")
	void NextGeneration();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnEvaluateNetworks();

	FTimerHandle NetworkUpdateTimerHandle;
};
