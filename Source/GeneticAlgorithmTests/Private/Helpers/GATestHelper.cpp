// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Helpers/GATestHelper.h"

#include "Components/GenomeComponents.h"

namespace GATestHelper
{
	void InitializeRandomGenome(TArray<char>& Storage, FGenomeCharViewComponent& ViewComp, int32 Length, FRandomStream& Rng)
	{
		Storage.SetNum(Length, EAllowShrinking::No);
		for (int32 i = 0; i < Length; ++i)
		{
			const uint8 v = static_cast<uint8>(Rng.RandRange(0, 255));
			Storage[i] = static_cast<char>(v);
		}
		ViewComp.Values = TArrayView<char>(Storage.GetData(), Storage.Num());
	}

	void ComputeBinaryFitness(entt::registry& Registry, const TArray<uint8>& TargetBytes)
	{
		const int32 N = TargetBytes.Num();
		auto View = Registry.view<FFitnessComponent, FGenomeCharViewComponent>();
		for (auto Entity : View)
		{
			FFitnessComponent& Fit = Registry.get<FFitnessComponent>(Entity);
			FGenomeCharViewComponent& ViewComp = Registry.get<FGenomeCharViewComponent>(Entity);
			const TArrayView<char>& Genes = ViewComp.Values;
			const int32 Count = FMath::Min(N, Genes.Num());
			int32 BitMatches = 0;
			for (int32 i = 0; i < Count; ++i)
			{
				const uint8 g = static_cast<uint8>(Genes[i]);
				const uint8 t = TargetBytes[i];
				const uint8 x = static_cast<uint8>(g ^ t);
				const int32 diffBits = FMath::CountBits(static_cast<uint32>(x));
				BitMatches += 8 - diffBits;
			}
			const float Score = static_cast<float>(BitMatches);
			Fit.Fitness.SetNum(1, EAllowShrinking::No);
			Fit.Fitness[0] = Score * Score; // square to amplify differences
			Fit.BuiltForFitnessIndex = 0;
		}
	}
}
