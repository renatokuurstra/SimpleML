//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Math/RandomStream.h"
#include "MutationCharGenomeSystem.generated.h"

struct FResetGenomeComponent;
struct FGenomeCharViewComponent;

/**
 * Mutates char-based genomes in-place by flipping individual bits with low probability.
 *
 * Efficiency: uses geometric skipping over bits to sample only flip locations.
 * Expected time is proportional to the number of flips (O(p * Nbits)) rather than
 * the total number of bits, which is beneficial for small probabilities.
 *
 * Stateless; only requires FGenomeCharViewComponent.
 */
UCLASS()
class GENETICALGORITHM_API UMutationCharGenomeSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	UMutationCharGenomeSystem()
	{
		RegisterComponent<FGenomeCharViewComponent>();
		RegisterComponent<FResetGenomeComponent>();
	}

	// Per-bit flip probability (default 1%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Mutation", meta=(ClampMin="0.0", ClampMax="1.0"))
	float BitFlipProbability = 0.01f;

	// Optional RNG seed for deterministic behavior (0 = use engine RNG)
	// RNG seeding policy: if RandomSeed != 0, we seed once and advance across updates; we do NOT reseed every Update.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Mutation")
	int32 RandomSeed = 0;

 virtual void Update(float DeltaTime = 0.0f) override;
private:
	FRandomStream Rng;
	bool bRngSeeded = false;
	bool bUseStream = false;
};
