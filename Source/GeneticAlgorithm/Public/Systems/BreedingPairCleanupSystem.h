//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/BreedingPairComponent.h"
#include "BreedingPairCleanupSystem.generated.h"

/**
 * Very simple system: destroys all entities that carry FBreedingPairComponent.
 * Why: Clean up transient pairing/link entities after they have been consumed.
 */
UCLASS()
class GENETICALGORITHM_API UBreedingPairCleanupSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	UBreedingPairCleanupSystem()
	{
		RegisterComponent<FBreedingPairComponent>();
	}

	virtual void Update(float DeltaTime = 0.0f) override;
};
