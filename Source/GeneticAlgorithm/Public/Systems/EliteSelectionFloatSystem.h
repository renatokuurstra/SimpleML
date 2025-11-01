//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "Systems/EliteSelectionBaseSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "EliteSelectionFloatSystem.generated.h"

/**
 * Selects and tags elites per fitness index for float genomes.
 */
UCLASS()
class GENETICALGORITHM_API UEliteSelectionFloatSystem : public UEliteSelectionBaseSystem
{
	GENERATED_BODY()
public:
	UEliteSelectionFloatSystem()
	{
		RegisterComponent<FGenomeFloatViewComponent>();
	}

protected:
	virtual bool IsCandidate(entt::entity E, const FFitnessComponent& Fit) const override;
	virtual void CopyGenomeToElite(entt::entity Winner, entt::entity Elite, int32 FitnessIndex) override;
};
