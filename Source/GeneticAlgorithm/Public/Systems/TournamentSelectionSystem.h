#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "TournamentSelectionSystem.generated.h"

struct FFitnessComponent;
struct FResetGenomeComponent;
struct FBreedingPairComponent;


/**
 * Tournament selection system.
 * Why: Select parents by sampling small tournaments over fitness values.
 * Stateless; operates only on component data.
 */
UCLASS()
class GENETICALGORITHM_API UTournamentSelectionSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	UTournamentSelectionSystem()
	{
		RegisterComponent<FFitnessComponent>();
		RegisterComponent<FResetGenomeComponent>();
		RegisterComponent<FBreedingPairComponent>();
	}

	// Selection parameters are configured per-system asset and treated as constants at runtime.
	// Unreal does not allow const UPROPERTY; we expose EditDefaultsOnly and never mutate them in code.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeneticAlgorithm|Selection|Tournament", meta=(ClampMin="2"))
	int32 TournamentSize = 3;

	// If true, entities can be sampled multiple times within a tournament.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeneticAlgorithm|Selection|Tournament")
	bool bWithReplacement = true;

	// Probability that the best candidate wins the tournament. Higher = stronger pressure.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeneticAlgorithm|Selection|Tournament", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SelectionPressure = 0.8f;

	// If true, higher fitness is better.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeneticAlgorithm|Selection|Tournament")
	bool bHigherIsBetter = true;

	// Per-parent chance to draw from the whole population (ignoring group) when selecting a parent.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeneticAlgorithm|Selection|Tournament", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CrossGroupParentChance = 0.1f;

	// Optional seed for deterministic tests (0 means use engine RNG).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeneticAlgorithm|Selection|Tournament")
	int32 RandomSeed = 0;

	virtual void Update(float DeltaTime = 0.0f) override;

private:
	struct FEntityRefFitness
	{
		float Value = 0.0f;
		int32 Order = 0;
		entt::entity Entity = entt::null;
	};

	// Reusable caches to avoid per-tick allocations
	mutable TArray<entt::entity> ResetTargets;
	mutable TArray<TArray<FEntityRefFitness>> GroupBuckets; // indexed by fitness dimension
	mutable TArray<FEntityRefFitness> GlobalBucket;
	mutable TArray<int32> ScratchIndices;

	// Helper: run a tournament on a bucket and return winning entity, or entt::null
	entt::entity RunTournament(const TArray<FEntityRefFitness>& Bucket, FRandomStream* Rng);
};
