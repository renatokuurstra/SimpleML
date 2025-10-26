#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/GenomeComponents.h"
#include "SelectionSystems.generated.h"

/**
 * Tournament selection system boilerplate.
 * Operates over fitness values. Concrete selection implementation will be added later.
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

	virtual void Update(float DeltaTime = 0.0f) override;
};

/**
 * Elite selection system boilerplate.
 * Operates over fitness values to select top performers.
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

	virtual void Update(float DeltaTime = 0.0f) override;
};