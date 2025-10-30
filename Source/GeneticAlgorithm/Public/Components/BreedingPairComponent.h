#pragma once

#include "CoreMinimal.h"
#include "BreedingPairComponent.generated.h"

/**
 * Holds references to the two parent entities and their resulting child entity.
 * Why: Produced by selection/reproduction pipeline to link lineage.
 * Note: Pure data (POD-style). Uses raw entity IDs (uint32) to avoid UObject coupling.
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FBreedingPairComponent
{
	GENERATED_BODY()

	// First parent entity selected by the system (0 means invalid/null).
	UPROPERTY(EditAnywhere, Category = "GeneticAlgorithm")
	uint32 ParentA = 0u;

	// Second parent entity selected by the system (0 means invalid/null).
	UPROPERTY(EditAnywhere, Category = "GeneticAlgorithm")
	uint32 ParentB = 0u;
};
