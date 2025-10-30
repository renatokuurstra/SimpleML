//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
// GeneticAlgorithm module (within SimpleML): Elite solution owning components
// Why: Store copies of elite genomes so we can preserve and reuse the best individuals across generations
#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "EliteComponents.generated.h"

/**
 * Owns a contiguous buffer of float values for an elite solution.
 * Rationale: We copy from a non-owning genome view (e.g., FGenomeFloatViewComponent) into this
 * owning buffer when an entity is selected as an elite. This avoids lifetime issues and allows
 * reusing entities for up to Max-X elites by overwriting the arrays without recreating entities.
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FEliteSolutionFloatComponent
{
	GENERATED_BODY()

	// Owning storage for the elite's genome weights/parameters
	UPROPERTY(EditAnywhere, Category = "GeneticAlgorithm|Elite")
	TArray<float> Values;
};

/**
 * Owns a contiguous buffer of byte values for an elite solution.
 * Mirrors the float version for genomes represented as bytes.
 * Note: UPROPERTY does not support char element type; use int8 for byte storage.
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FEliteSolutionCharComponent
{
	GENERATED_BODY()

	// Owning storage for the elite's genome bytes
	UPROPERTY(EditAnywhere, Category = "GeneticAlgorithm|Elite")
	TArray<int8> Values;
};


/**
 * Tag component assigning an elite entity to a specific fitness index group.
 * Used internally to keep separate pools of elites per fitness index.
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FEliteGroupIndex
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "GeneticAlgorithm|Elite")
	int32 FitnessIndex = 0;
};
