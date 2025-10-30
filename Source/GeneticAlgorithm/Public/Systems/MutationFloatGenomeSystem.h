//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/GenomeComponents.h"
#include "MutationFloatGenomeSystem.generated.h"

/**
 * Boilerplate mutation system acting on FGenomeFloatViewComponent.
 * Logic will be filled later; for now, it only declares dependencies and an empty Update.
 */
UCLASS()
class GENETICALGORITHM_API UMutationFloatGenomeSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	UMutationFloatGenomeSystem()
	{
		// Declare we require the float genome view
		RegisterComponent<FGenomeFloatViewComponent>();
	}

	virtual void Update(float DeltaTime = 0.0f) override;
};