//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Math/RandomStream.h"
#include "MutationFloatGenomeSystem.generated.h"

struct FResetGenomeComponent;
struct FGenomeFloatViewComponent;

/**
 * Mutates float genomes in-place.
 *
 * Operations, in order:
 * 1) Apply per-value multiplicative noise: v *= (1 + u), where u ~ U[-PerValueDeltaPercent, +PerValueDeltaPercent].
 * 2) Roll per-entity random mutation with probability RandomMutationChance.
 * 3) If triggered, reset a random number of weights: N ~ U[0, RandomResetMaxPercent * Count], clamped to at least 1.
 *    Each reset index is unique; values are sampled in [RandomResetMin, RandomResetMax].
 *
 * Stateless; only requires FGenomeFloatViewComponent.
 */
UCLASS()
class GENETICALGORITHM_API UMutationFloatGenomeSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	UMutationFloatGenomeSystem()
	{
		RegisterComponent<FGenomeFloatViewComponent>();
		RegisterComponent<FResetGenomeComponent>();
	}

	// ±X% multiplicative noise per float (default 2.5%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Mutation", meta=(ClampMin="0.0"))
	float PerValueDeltaPercent = 0.025f;

	// Per-genome probability to perform random resets (default 5%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Mutation", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RandomMutationChance = 0.05f;

	// Upper bound for fraction of weights to reset when random mutation triggers (default equals 2.5%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Mutation", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RandomResetMaxPercent = 0.025f;

	// Range used when resetting selected weights
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Mutation")
	float RandomResetMin = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Mutation")
	float RandomResetMax = 1.0f;

	// Optional RNG seed for deterministic behavior (0 = use engine RNG)
	// Seeding policy: if RandomSeed != 0, we seed once in Initialize/first Update and advance across updates; no reseed per tick.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Mutation")
	int32 RandomSeed = 0;

 virtual void Update(float DeltaTime = 0.0f) override;
private:
	FRandomStream Rng;
	bool bRngSeeded = false;
	bool bUseStream = false;
};