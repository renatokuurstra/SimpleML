//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "GAStalenessSystem.generated.h"

/**
 * UGAStalenessSystem
 * Monitors fitness progress per population and triggers "Nukes" (resets) for stale populations.
 * Also performs Pioneer Injection: copying the global best genome into the stale population.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class SPLINECIRCUITTRAINER_API UGAStalenessSystem : public UEcsSystem
{
	GENERATED_BODY()

public:
	UGAStalenessSystem();

	virtual void Update_Implementation(float DeltaTime) override;

private:
	/** Tracks historical total fitness per population index to detect staleness. */
	TMap<int32, TArray<float>> PopulationFitnessHistory;
};
