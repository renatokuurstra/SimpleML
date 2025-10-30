// GeneticAlgorithm module (within SimpleML): ECS components for genomes and fitness
// Why: Provide POD-style data containers for GA systems to operate on.
#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "GenomeComponents.generated.h"

/**
 * Non-owning view into a contiguous float genome buffer.
 * Note: Not marked as UPROPERTY on purpose (TArrayView is non-reflected and non-owning).
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FGenomeFloatViewComponent
{
	GENERATED_BODY()

	// Points to external storage owned elsewhere (e.g., a big SoA buffer)
	TArrayView<float> Values;
};

/**
 * Non-owning view into a contiguous char genome buffer.
 * Using 'char' for byte-oriented genome representations.
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FGenomeCharViewComponent
{
	GENERATED_BODY()

	TArrayView<char> Values;
};

/**
 * Owns fitness scores per entity (can be one score per entity or a small vector per entity; here a single array aligned to entity order).
 * Systems should ensure the array length matches the number of relevant entities.
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FFitnessComponent
{
	GENERATED_BODY()

	// Owning fitness storage to support serialization and editor inspection if later desired
	UPROPERTY(EditAnywhere, Category = "GeneticAlgorithm")
	TArray<float> Fitness;

	// Index of the fitness dimension this entity was built/optimized for. -1 means no specific index.
	UPROPERTY(EditAnywhere, Category = "GeneticAlgorithm")
	int32 BuiltForFitnessIndex = -1;
};

/**
 * Tag component: marks entities whose genome should be rebuilt/reset.
 * Stateless POD; presence implies action.
 */
USTRUCT(BlueprintType)
struct GENETICALGORITHM_API FResetGenomeComponent
{
	GENERATED_BODY()
	// Intentionally empty
};
