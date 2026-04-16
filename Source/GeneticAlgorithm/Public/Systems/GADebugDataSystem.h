//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/GenomeComponents.h"
#include "GADebugDataSystem.generated.h"

/**
 * System that collects data for GeneticAlgorithm debugging.
 */
UCLASS()
class GENETICALGORITHM_API UGADebugDataSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UGADebugDataSystem();

	virtual void Update_Implementation(float DeltaTime) override;

private:
	float SampleTimer = 0.0f;
	float SampleInterval = 1.0f; // Sample once per second
};
