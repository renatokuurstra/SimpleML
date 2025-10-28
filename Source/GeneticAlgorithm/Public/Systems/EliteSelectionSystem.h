#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/GenomeComponents.h"
#include "EliteSelectionSystem.generated.h"

/**
 * Elite selection system.
 * Why: Select the top-performing entities based on fitness.
 * Stateless; operates on component data only.
 */
UCLASS()
class GENETICALGORITHM_API UEliteSelectionSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	UEliteSelectionSystem()
	{
		RegisterComponent<FFitnessComponent>();
	}

	// Number of elite individuals to select per generation (treated as a constant runtime parameter).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeneticAlgorithm|Selection|Elite", meta=(ClampMin="1"))
	int32 EliteCount = 4;

	// If true, higher fitness is better; otherwise lower is better.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeneticAlgorithm|Selection|Elite")
	bool bHigherIsBetter = true;

	virtual void Update(float DeltaTime = 0.0f) override;
};
