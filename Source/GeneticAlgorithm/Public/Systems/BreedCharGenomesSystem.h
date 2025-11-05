//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/BreedingPairComponent.h"
#include "BreedCharGenomesSystem.generated.h"

/**
 * System that breeds char/byte genomes by randomly taking each gene from either parent.
 *
 * Processing pattern mirrors UBreedFloatGenomesSystem:
 * - Iterate in-order over entities with FResetGenomeComponent + FGenomeCharViewComponent.
 * - Consume one FBreedingPairComponent entity per reset entity.
 * - For each gene, pick from ParentA or ParentB with equal probability.
 * - Inner loop uses 32-bit random masks to batch 32 gene picks per RNG call.
 *
 * Notes:
 * - This system does not destroy FBreedingPairComponent entities; use UBreedingPairCleanupSystem afterwards.
 */
UCLASS()
class GENETICALGORITHM_API UBreedCharGenomesSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	UBreedCharGenomesSystem()
	{
		RegisterComponent<FResetGenomeComponent>();
		RegisterComponent<FGenomeCharViewComponent>();
		RegisterComponent<FBreedingPairComponent>();
	}

	// Optional RNG seed for deterministic behavior (0 = use engine RNG)
	// Seeding policy: if RandomSeed != 0, we seed once and advance across updates;
	// do not reseed every tick to avoid degenerate identical outcomes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Breeding|Char")
	int32 RandomSeed = 0;

	virtual void Update(float DeltaTime = 0.0f) override;
private:
	FRandomStream Rng;
	bool bRngSeeded = false;
	bool bUseStream = false;
};
