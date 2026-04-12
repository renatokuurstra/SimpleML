//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleTrainerDebugSystem.generated.h"

/**
 * System that visualizes GeneticAlgorithm debug information using SlateIM.
 */
UCLASS()
class SPLINECIRCUITTRAINER_API UVehicleTrainerDebugSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleTrainerDebugSystem();

	virtual void Initialize_Implementation(AEcsContext* InContext) override;
	virtual void Update_Implementation(float DeltaTime) override;
	virtual void Deinitialize_Implementation() override;

private:
	UFUNCTION()
	void DrawDebugUI();

	FTimerHandle DebugTimerHandle;

	// Cached data for visualization
	int32 CachedEliteCount = 0;
	TArray<float> CachedEliteFitness;
	TArray<float> CachedBreedingPairsFitness;
	TArray<float> CachedAllSolutionsFitness;
	int32 CachedResetCount = 0;
};
