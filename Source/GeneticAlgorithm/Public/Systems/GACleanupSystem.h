//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/BreedingPairComponent.h"
#include "Components/GenomeComponents.h"
#include "GACleanupSystem.generated.h"

/**
 * GA end-of-step cleanup:
 * - Destroys all transient entities that carry FBreedingPairComponent.
 * - Removes FResetGenomeComponent tags from entities so the user can re-apply them next generation.
 */
UCLASS()
class GENETICALGORITHM_API UGACleanupSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	UGACleanupSystem()
	{
		RegisterComponent<FBreedingPairComponent>();
		RegisterComponent<FResetGenomeComponent>();
	}

	virtual void Update(float DeltaTime = 0.0f) override;
};
