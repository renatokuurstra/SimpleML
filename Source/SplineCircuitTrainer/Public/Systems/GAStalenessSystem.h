//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "GAStalenessSystem.generated.h"

/**
 * UGAStalenessSystem
 * Monitors fitness progress per population and triggers "Nukes" (resets) for stale populations.
 *
 * Algorithm:
 * 1. Every tick, compute total elite fitness per population.
 * 2. Identify the population with the LOWEST total elite fitness.
 * 3. Check staleness history for ONLY that population (using last N total fitness values).
 * 4. If stale, nuke that single population:
 *    a. Tag ALL non-elite entities for reset (position + training data).
 *    b. Re-randomize NN weights for ALL non-elite entities in that population.
 *    c. Reset ALL elite fitness in that population to neutral.
 *    d. Destroy ALL elite entities in that population.
 *    e. Create new elite entities for ALL elite slots, each seeded with the global best genome.
 * 5. Set a global cooldown timer -- no further nukes until it expires.
 *
 * Key design decisions:
 * - Only the LOWEST-fitness population is ever evaluated for staleness.
 *   Higher-fitness populations are never touched, even if they're technically "stale".
 * - Global cooldown ensures only one nuke happens at a time across all populations.
 * - Pioneer injection replaces ALL elite slots (not just one), seeding the entire
 *   elite pool with the global best genome while the population entities provide diversity.
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
