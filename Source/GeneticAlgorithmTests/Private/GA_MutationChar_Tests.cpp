// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License.
// Unit tests for UMutationCharGenomeSystem.

#include "CoreMinimal.h"
#include "CQTest.h"

#include "entt/entt.hpp"

#include "Systems/MutationCharGenomeSystem.h"
#include "Components/GenomeComponents.h"

TEST_CLASS(SimpleML_GA_MutationChar_Tests, "SimpleML.GA.MutationChar")
{
	// ECS registry per test
	entt::registry Registry;

	// Backing storage for genome view
	TArray<int8> Genome;
	TArray<int8> Original;

	UMutationCharGenomeSystem* Mutator = nullptr;

	BEFORE_EACH()
	{
		// Create one entity with ~1000 bytes genome (use 1024 for alignment)
		const int32 NumBytes = 1024;
		Genome.SetNumZeroed(NumBytes, EAllowShrinking::No);
		Original.SetNumZeroed(NumBytes, EAllowShrinking::No);

		// Fill with alternating pattern to avoid bias (01010101 / 10101010)
		for (int32 i = 0; i < NumBytes; ++i)
		{
			Genome[i] = (i & 1) ? static_cast<int8>(0xAA) : static_cast<int8>(0x55);
			Original[i] = Genome[i];
		}

		const entt::entity E = Registry.create();
		FGenomeCharViewComponent View{};
		Registry.emplace<FGenomeCharViewComponent>(E, View);
		// Bind view to backing storage
		Registry.get<FGenomeCharViewComponent>(E).Values = TArrayView<char>(reinterpret_cast<char*>(Genome.GetData()), Genome.Num());
		// Mark as reset so mutation system processes it
		Registry.emplace<FResetGenomeComponent>(E, FResetGenomeComponent{});

		// Create and configure mutator
		Mutator = NewObject<UMutationCharGenomeSystem>();
		Mutator->Initialize(Registry);
		Mutator->BitFlipProbability = 0.20f; // 20%
		Mutator->RandomSeed = 42; // deterministic
	}

	AFTER_EACH()
	{
		if (Mutator)
		{
			Mutator->Deinitialize();
			Mutator = nullptr;
		}
		Registry.clear();
		Genome.Reset();
		Original.Reset();
	}

	static int32 PopCount(uint8 x)
	{
		// Kernighan's bit count
		int32 c = 0;
		while (x)
		{
			x &= static_cast<uint8>(x - 1);
			++c;
		}
		return c;
	}

	TEST_METHOD(Mutates_At_Least_15_Percent_Bits_With_P20)
	{
		// Act: run one mutation update
		Mutator->Update(0.0f);

		// Assert: count flipped bits
		int64 flipped = 0;
		const int64 totalBits = static_cast<int64>(Genome.Num()) * 8;
		for (int32 i = 0; i < Genome.Num(); ++i)
		{
			const uint8 before = static_cast<uint8>(Original[i]);
			const uint8 after  = static_cast<uint8>(Genome[i]);
			flipped += PopCount(before ^ after);
		}

		const double fraction = (totalBits > 0) ? static_cast<double>(flipped) / static_cast<double>(totalBits) : 0.0;
		UE_LOG(LogTemp, Display, TEXT("MutationChar: flipped=%lld totalBits=%lld fraction=%.3f (p=0.20)"), flipped, totalBits, fraction);

		const int64 minFlips = static_cast<int64>(FMath::FloorToDouble(0.15 * static_cast<double>(totalBits)));
		ASSERT_THAT(IsTrue(flipped >= minFlips));
	}
};
