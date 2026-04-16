//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "UI/VehicleTrainerDebugWidget.h"
#include "VehicleTrainerManager.generated.h"

class AVehicleTrainerContext;
class UVehicleTrainerConfig;
class UVehicleTrainerDebugWidget;

/**
 * AVehicleTrainerManager
 * Manages multiple independent VehicleTrainerContext instances.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API AVehicleTrainerManager : public AActor
{
	GENERATED_BODY()

public:
	AVehicleTrainerManager();

	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override { return false; }

	/** Configuration used for each context */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TObjectPtr<UVehicleTrainerConfig> TrainerConfig;

	/** Actor containing the spline circuit to be used by all contexts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TObjectPtr<AActor> CircuitActor;

	/** Number of independent contexts to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer", meta = (ClampMin = "1"))
	int32 NumContexts = 1;

	/** Optional seed for each context. If not provided, seeds will be assigned progressively. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TArray<int32> ContextSeeds;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	/** Spawns and initializes all contexts */
	UFUNCTION(BlueprintCallable, Category = "Trainer")
	void SpawnContexts();

	/** Toggle debug UI visibility */
	UFUNCTION()
	void ToggleDebugUI();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer|Debug")
	TSubclassOf<UVehicleTrainerDebugWidget> DebugWidgetClass;

private:
	UPROPERTY()
	FTimerHandle UpdateTimerHandle;

	void OnUpdate();

	void LogEliteFitness();

	UPROPERTY()
	TArray<TObjectPtr<AVehicleTrainerContext>> Contexts;

	UPROPERTY()
	TObjectPtr<UVehicleTrainerDebugWidget> DebugWidget;

	void UpdateDebugWidget();

	/** Checks each context for staleness and performs a nuke if necessary. */
	void CheckForStalenessAndNuke();
};
