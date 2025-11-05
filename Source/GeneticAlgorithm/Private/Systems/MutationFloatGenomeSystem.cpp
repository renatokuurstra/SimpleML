// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/MutationFloatGenomeSystem.h"
#include "Math/UnrealMathUtility.h"
#include "Components/GenomeComponents.h"
#include "Containers/Set.h"

void UMutationFloatGenomeSystem::Update(float /*DeltaTime*/)
{
	auto& Registry = GetRegistry();

	// Iterate directly over entities with a float genome view
	auto View = GetView<FGenomeFloatViewComponent, FResetGenomeComponent>();
	if (View.begin() == View.end())
	{
		return;
	}

	// RNG policy: seed once and advance across updates if RandomSeed != 0
	if (!bRngSeeded && RandomSeed != 0)
	{
		Rng.Initialize(RandomSeed);
		bRngSeeded = true;
		bUseStream = true;
	}
	FRandomStream* RngPtr = bUseStream ? &Rng : nullptr;

	// Sanitize parameters
	const float DeltaPct = FMath::Max(0.0f, PerValueDeltaPercent);
	const float ResetFracMax = FMath::Clamp(RandomResetMaxPercent, 0.0f, 1.0f);
	float ResetMin = RandomResetMin;
	float ResetMax = RandomResetMax;
	if (ResetMin > ResetMax)
	{
		Swap(ResetMin, ResetMax);
	}

	for (auto It = View.begin(), End = View.end(); It != End; ++It)
	{
		const entt::entity Entity = *It;
		FGenomeFloatViewComponent& ViewComp = Registry.get<FGenomeFloatViewComponent>(Entity);
		TArrayView<float> Values = ViewComp.Values;
		const int32 Count = Values.Num();
		if (Count <= 0)
		{
			continue;
		}

		// 1) Per-value multiplicative noise: v *= (1 + u), with u in [-DeltaPct, +DeltaPct]
		for (int32 i = 0; i < Count; ++i)
		{
			const float u01 = RngPtr ? RngPtr->FRand() : FMath::FRand();
			const float u = (u01 * 2.0f - 1.0f) * DeltaPct; // uniform in [-DeltaPct, +DeltaPct]
			Values[i] *= (1.0f + u);
		}

		// 2) Roll for random mutation
		const float roll = RngPtr ? RngPtr->FRand() : FMath::FRand();
		if (roll <= RandomMutationChance)
		{
			// 3) Determine how many unique weights to reset
			const float kUpperF = ResetFracMax * static_cast<float>(Count);
			const int32 kUpper = FMath::FloorToInt(kUpperF);
			int32 K;
			if (kUpper <= 0)
			{
				K = 1; // min 1 weight no matter what
			}
			else
			{
				const int32 r = RngPtr ? RngPtr->RandRange(0, kUpper) : FMath::RandRange(0, kUpper);
				K = FMath::Clamp(r, 1, Count);
			}

			// Sample K unique indices and reset them to U[ResetMin, ResetMax]
			TSet<int32> Picked;
			Picked.Reserve(K);
			while (Picked.Num() < K)
			{
				const int32 idx = RngPtr ? RngPtr->RandRange(0, Count - 1) : FMath::RandRange(0, Count - 1);
				if (Picked.Contains(idx))
				{
					continue; // already present
				}
				Picked.Add(idx);
				const float v01 = RngPtr ? RngPtr->FRand() : FMath::FRand();
				Values[idx] = FMath::Lerp(ResetMin, ResetMax, v01);
			}
		}
	}
}
