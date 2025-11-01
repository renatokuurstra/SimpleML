//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
// GeneticAlgorithm module (within SimpleML): Elite solution owning components
// Why: Store copies of elite genomes so we can preserve and reuse the best individuals across generations
#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "EliteComponents.generated.h"

/**
 * Elite tag: marks entities that are elites (separate from the regular population).
 * Systems should use base components for logic and optionally filter by this tag.
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FEliteTagComponent
{
	GENERATED_BODY()
	// Intentionally empty
};

/**
 * Owning storage for elite float genomes.
 * Why: Elite entities must be distinct and own their copied genome data; base view points to this buffer.
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FEliteOwnedFloatGenome
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "GeneticAlgorithm|Elite")
	TArray<float> Values;
};

/**
 * Owning storage for elite char/byte genomes.
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FEliteOwnedCharGenome
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "GeneticAlgorithm|Elite")
	TArray<int8> Values;
};
