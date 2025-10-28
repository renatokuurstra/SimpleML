#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/GenomeComponents.h"
#include "TournamentSelectionSystem.generated.h"

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

	// Optional seed for deterministic tests (0 means use engine RNG).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeneticAlgorithm|Selection|Tournament")
	int32 RandomSeed = 0;

	virtual void Update(float DeltaTime = 0.0f) override;
};
