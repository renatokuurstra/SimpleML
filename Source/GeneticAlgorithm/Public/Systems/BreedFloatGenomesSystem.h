//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/BreedingPairComponent.h"
#include "BreedFloatGenomesSystem.generated.h"

/**
 * System that breeds float genomes using SBX (Simulated Binary Crossover).
 *
 * It pairs, in-order, entities marked with FResetGenomeComponent (+ float view)
 * with entities carrying FBreedingPairComponent. For each pair, it reads the two
 * parents' float genome views and writes a new child genome into the reset entity.
 *
 * Notes:
 * - This system does not destroy FBreedingPairComponent entities; use UBreedingPairCleanupSystem after it.
* - Iterates directly over views (no per-tick caches) and consumes one FBreedingPair per reset entity.
*/
UCLASS()
class GENETICALGORITHM_API UBreedFloatGenomesSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	UBreedFloatGenomesSystem()
	{
		RegisterComponent<FResetGenomeComponent>();
		RegisterComponent<FGenomeFloatViewComponent>();
		RegisterComponent<FBreedingPairComponent>();
	}

	// SBX parameters
	// Probability of applying crossover per gene. Otherwise a gene is copied from a random parent.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Breeding|SBX", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CrossoverProbability = 0.9f;

	// Distribution index (eta) controls exploration vs. exploitation; higher = children closer to parents.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Breeding|SBX", meta=(ClampMin="0.0"))
	float Eta = 15.0f;

	// Clamp resulting child genes to a range (useful when genomes represent bounded parameters)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Breeding|SBX")
	bool bClampChildren = true;

	// Clamp range (used only when bClampChildren = true)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Breeding|SBX")
	float ClampMin = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Breeding|SBX")
	float ClampMax = 1.0f;

	// Optional RNG seed for deterministic behavior (0 = use engine RNG)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Breeding|SBX")
	int32 RandomSeed = 0;

	virtual void Update(float DeltaTime = 0.0f) override;

private:
	// Per-gene SBX child sampling. Returns one child value produced from parents x1, x2.
	float SampleSbxChild(float X1, float X2, float U, float EtaLocal, bool bPickFirst) const;
};
