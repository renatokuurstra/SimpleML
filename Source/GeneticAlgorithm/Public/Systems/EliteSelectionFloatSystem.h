#pragma once

#include "CoreMinimal.h"
#include "Systems/EliteSelectionBaseSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
#include "EliteSelectionFloatSystem.generated.h"

/**
 * Selects elites per fitness index for float genomes and copies their genomes into owning elite components.
 */
UCLASS()
class GENETICALGORITHM_API UEliteSelectionFloatSystem : public UEliteSelectionBaseSystem
{
	GENERATED_BODY()
public:
	UEliteSelectionFloatSystem()
	{
		RegisterComponent<FGenomeFloatViewComponent>();
		RegisterComponent<FEliteSolutionFloatComponent>();
		RegisterComponent<FEliteGroupIndex>();
	}

protected:
	virtual void Update(float DeltaTime = 0.0f) override;
	virtual void SaveEliteGenome(entt::entity SourceEntity, entt::entity EliteEntity) override;
};
