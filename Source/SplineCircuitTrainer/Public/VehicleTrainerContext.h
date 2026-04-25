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

	/** Unique seed for random-dependent systems in this context */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	int32 RandomSeed = 0;

	/** Index assigned by the manager to identify this context */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	int32 ContextIndex = -1;

	/** Next time a nuke operation is eligible globally. Single cooldown across all populations. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Trainer")
	float NextNukeAvailableTime = 0.0f;

	/** Helper function to retrieve the spline component from the CircuitActor */
	UFUNCTION(BlueprintCallable, Category = "Trainer")
	USplineComponent* GetCircuitSpline() const;

	/** Forces re-initialization of systems from the current TrainerConfig */
	UFUNCTION(BlueprintCallable, Category = "Trainer")
	void InitializeSystemsFromConfig();

	UFUNCTION(BlueprintCallable, Category = "Trainer")
	void ToggleDebugUI();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UVehicleTrainerDebugWidget> DebugWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UVehicleTrainerDebugWidget> DebugWidget;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

private:
	void OnEvaluateNetworks();

	FTimerHandle NetworkUpdateTimerHandle;
};
