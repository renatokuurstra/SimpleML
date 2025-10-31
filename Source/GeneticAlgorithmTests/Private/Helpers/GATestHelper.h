// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "entt/entt.hpp"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"

// Forward declarations for components used by helpers
struct FGenomeCharViewComponent;

namespace GATestHelper
{
	/**
	 * Bind a genome view to owned storage and random-initialize bytes.
	 */
	void InitializeRandomGenome(TArray<char>& Storage, FGenomeCharViewComponent& ViewComp, int32 Length, FRandomStream& Rng);

	/**
	 * Compute fitness as per-bit matches against TargetBytes, squared.
	 * - Each matching bit adds 1.
	 * - The total is squared to increase selection pressure.
	 * Writes into entities' FFitnessComponent[0] and sets BuiltForFitnessIndex = 0.
	 */
	void ComputeBinaryFitness(entt::registry& Registry, const TArray<uint8>& TargetBytes);
}
