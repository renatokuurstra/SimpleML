// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
// GA Integration Tests file: end-to-end CQTest for binary (char) genomes.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "Containers/StringConv.h"

#include "entt/entt.hpp"

// GA components and systems
#include "Components/GenomeComponents.h"
#include "Components/BreedingPairComponent.h"
#include "Components/EliteComponents.h"
#include "Systems/EliteSelectionCharSystem.h"
#include "Systems/TournamentSelectionSystem.h"
#include "Systems/BreedCharGenomesSystem.h"
#include "Systems/MutationCharGenomeSystem.h"
#include "Systems/BreedingPairCleanupSystem.h"

// Local helpers for GA integration tests
#include "Helpers/GATestHelper.h"

TEST_CLASS(SimpleML_GA_Binary_E2E, "SimpleML.GA.Integration")
{
	// Shared per-test state
	entt::registry Registry;

	// Owned genome storage per entity (to back non-owning TArrayView in components)
	TArray<TArray<char>> Genomes;

	// Target string bytes
	TArray<uint8> TargetBytes;
	int32 GenomeLen = 0;
	int32 PopulationSize = 50;
	int32 MaxGenerations = 600;
	float BottomResetFraction = 0.4f;
	int32 Seed = 1337;

 // Systems
	UEliteSelectionCharSystem* Elite = nullptr;
	UTournamentSelectionSystem* Selection = nullptr;
	UBreedCharGenomesSystem* Breeder = nullptr;
	UMutationCharGenomeSystem* Mutator = nullptr;
	UBreedingPairCleanupSystem* Cleanup = nullptr;

	BEFORE_ALL()
	{
	}

	BEFORE_EACH()
	{
		// Parameters
		const FString TargetString = TEXT("Testing GA binaryblob");
		// Convert wide string to UTF-8 bytes; our genome is byte-based and uses ASCII here.
		{
			FTCHARToUTF8 Converter(*TargetString);
			GenomeLen = Converter.Length();
			TargetBytes.Reset(GenomeLen);
			TargetBytes.AddUninitialized(GenomeLen);
			FMemory::Memcpy(TargetBytes.GetData(), (const void*)Converter.Get(), GenomeLen);
		}

		// Prepare genome storage
		Genomes.SetNum(PopulationSize, EAllowShrinking::No);

		FRandomStream Rng(Seed);
		for (int32 i = 0; i < PopulationSize; ++i)
		{
			const entt::entity E = Registry.create();

			FGenomeCharViewComponent ViewComp{};
			FFitnessComponent Fit{};
			Fit.Fitness.SetNum(1, EAllowShrinking::No);
			Fit.Fitness[0] = 0.0f;
			Fit.BuiltForFitnessIndex = 0;

			Registry.emplace<FGenomeCharViewComponent>(E, ViewComp);
			Registry.emplace<FFitnessComponent>(E, Fit);

			GATestHelper::InitializeRandomGenome(Genomes[i], Registry.get<FGenomeCharViewComponent>(E), GenomeLen, Rng);
		}

		Elite = NewObject<UEliteSelectionCharSystem>();
		Elite->Initialize(Registry);
		Elite->EliteCount = 3;
		Elite->bHigherIsBetter = true;
		
		Selection = NewObject<UTournamentSelectionSystem>();
		Selection->Initialize(Registry);
		Selection->TournamentSize = 8;
		Selection->SelectionPressure = 0.7f;
		Selection->bHigherIsBetter = true;
		Selection->CrossGroupParentChance = 0.0f; // single group
		Selection->RandomSeed = Seed + 1;

		Breeder = NewObject<UBreedCharGenomesSystem>();
		Breeder->Initialize(Registry);
		Breeder->RandomSeed = Seed + 2;

		Mutator = NewObject<UMutationCharGenomeSystem>();
		Mutator->Initialize(Registry);
		Mutator->BitFlipProbability = 0.025f; // 2.5% per-bit
		Mutator->RandomSeed = Seed + 3;

		Cleanup = NewObject<UBreedingPairCleanupSystem>();
		Cleanup->Initialize(Registry);
	}

	AFTER_EACH()
	{
		if (Elite) { Elite->Deinitialize(); Elite = nullptr; }
		if (Selection) { Selection->Deinitialize(); Selection = nullptr; }
		if (Breeder) { Breeder->Deinitialize(); Breeder = nullptr; }
		if (Mutator) { Mutator->Deinitialize(); Mutator = nullptr; }
		if (Cleanup) { Cleanup->Deinitialize(); Cleanup = nullptr; }

		// Destroy all entities and clear storage
		Registry.clear();
		Genomes.Reset();
		TargetBytes.Reset();
	}

	AFTER_ALL()
	{
	}

	TEST_METHOD(Converges_To_Target_String_Binary)
	{
		// GA loop
		bool bMatched = false;
		TArray<char> BestSoFar;
		BestSoFar.SetNum(GenomeLen, EAllowShrinking::No);

		// Helper: convert bytes to space-separated uppercase hex string
		auto ToHex = [](const uint8* Data, int32 Len) -> FString
		{
			FString Out;
			Out.Reserve(Len * 3);
			for (int32 i = 0; i < Len; ++i)
			{
				Out += FString::Printf(TEXT("%02X"), Data[i]);
				if (i + 1 < Len) { Out += TEXT(" "); }
			}
			return Out;
		};

		for (int32 Gen = 0; Gen < MaxGenerations; ++Gen)
		{
			// 1) Evaluate fitness
			GATestHelper::ComputeBinaryFitness(Registry, TargetBytes);

			// 1b) Mark only the bottom fraction of entities as reset candidates for next generation
			{
				// Build list of non-elite population with their fitness and insertion order for stable ties
				struct FEntFit { entt::entity E; float V; int32 Order; };
				TArray<FEntFit> Ranked;
				Ranked.Reserve(PopulationSize);
				int32 Order = 0;
				auto PopView = Registry.view<FFitnessComponent>(entt::exclude_t<FEliteTagComponent>{});
				for (auto E : PopView)
				{
					const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
					const float V = (Fit.Fitness.Num() > 0) ? Fit.Fitness[0] : -FLT_MAX;
					Ranked.Add({E, V, Order});
					++Order;
				}
				// Sort ascending by fitness (worse first), tie-break by order
				Ranked.Sort([](const FEntFit& A, const FEntFit& B)
				{
					if (A.V != B.V) return A.V < B.V; // lower fitness first
					return A.Order < B.Order;
				});
				// Determine how many to reset this generation
				const int32 Desired = FMath::Clamp(FMath::FloorToInt(static_cast<float>(PopulationSize) * BottomResetFraction + 1e-6f), 1, PopulationSize);
				const int32 ResetCount = FMath::Min(Desired, Ranked.Num());
				
				// Mark bottom ResetCount as reset
				for (int32 i = 0; i < ResetCount; ++i)
				{
					const entt::entity E = Ranked[i].E;
					if (!Registry.all_of<FResetGenomeComponent>(E))
					{
						Registry.emplace<FResetGenomeComponent>(E);
					}
				}
			}

			// 2) Track best genome and check for match
			float BestFitnessLocal = -FLT_MAX;
			entt::entity BestEntity = entt::null;
			auto View = Registry.view<FFitnessComponent, FGenomeCharViewComponent>();
			for (auto E : View)
			{
				const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
				const float F0 = Fit.Fitness.Num() > 0 ? Fit.Fitness[0] : -FLT_MAX;
				if (F0 > BestFitnessLocal)
				{
					BestFitnessLocal = F0;
					BestEntity = E;
				}
			}
			if (BestEntity != entt::null)
			{
				const TArrayView<char>& ViewBest = Registry.get<FGenomeCharViewComponent>(BestEntity).Values;
				for (int32 i = 0; i < GenomeLen; ++i) { BestSoFar[i] = (i < ViewBest.Num()) ? ViewBest[i] : 0; }
				
				bool bEqual = true;
				for (int32 i = 0; i < GenomeLen; ++i)
				{
					if (static_cast<uint8>(BestSoFar[i]) != TargetBytes[i]) { bEqual = false; break; }
				}
				// Build hex strings for periodic and success logging
				const FString BestHex = ToHex(reinterpret_cast<const uint8*>(BestSoFar.GetData()), GenomeLen);
				const FString TargetHex = ToHex(TargetBytes.GetData(), GenomeLen);
				// Periodic debug log of best candidate every 10 generations (hex instead of string)
				if ((Gen % 10) == 0)
				{
					UE_LOG(LogTemp, Display, TEXT("[GA Binary Test] Gen=%d BestFitness=%f BestHex=%s TargetHex=%s"), Gen, BestFitnessLocal, *BestHex, *TargetHex);
				}
				if (bEqual)
				{
					// Log right before test success to capture converged genome in hex
					UE_LOG(LogTemp, Display, TEXT("[GA Binary Test] Before success Gen=%d BestFitness=%f BestHex=%s TargetHex=%s"), Gen, BestFitnessLocal, *BestHex, *TargetHex);
					bMatched = true;
					break;
				}
			}

			// 3) Elite selection → tournament selection → breeding → mutation → cleanup
			Elite->Update(0.0f);
			Selection->Update(0.0f);
			Breeder->Update(0.0f);
			Mutator->Update(0.0f);
			Cleanup->Update(0.0f);
		}

		// Final evaluation and assertion
		GATestHelper::ComputeBinaryFitness(Registry, TargetBytes);
		float BestFitness = -FLT_MAX;
		auto View2 = Registry.view<FFitnessComponent, FGenomeCharViewComponent>();
		for (auto E : View2)
		{
			const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
			const float F0 = Fit.Fitness.Num() > 0 ? Fit.Fitness[0] : -FLT_MAX;
			if (F0 > BestFitness)
			{
				BestFitness = F0;
			}
		}
		const float MaxBits = static_cast<float>(GenomeLen * 8);
		const float MaxFitness = MaxBits * MaxBits;
		const float Threshold = 0.95f * MaxFitness; // 95% of bits correct (squared)

		if (!bMatched)
		{
			// Recompute best genome to log its hex alongside target
			entt::entity BestEntityFinal = entt::null;
			float BestFitnessFinal = -FLT_MAX;
			for (auto E : View2)
			{
				const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
				const float F0 = Fit.Fitness.Num() > 0 ? Fit.Fitness[0] : -FLT_MAX;
				if (F0 > BestFitnessFinal)
				{
					BestFitnessFinal = F0;
					BestEntityFinal = E;
				}
			}
			if (BestEntityFinal != entt::null)
			{
				const TArrayView<char>& ViewBestFinal = Registry.get<FGenomeCharViewComponent>(BestEntityFinal).Values;
				for (int32 i = 0; i < GenomeLen; ++i) { BestSoFar[i] = (i < ViewBestFinal.Num()) ? ViewBestFinal[i] : 0; }
				const FString BestHexFinal = ToHex(reinterpret_cast<const uint8*>(BestSoFar.GetData()), GenomeLen);
				const FString TargetHexFinal = ToHex(TargetBytes.GetData(), GenomeLen);
				UE_LOG(LogTemp, Warning, TEXT("GA binary E2E: no exact match within %d generations. Best fitness: %f (threshold: %f, max: %f). BestHex=%s TargetHex=%s"), MaxGenerations, BestFitness, Threshold, MaxFitness, *BestHexFinal, *TargetHexFinal);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("GA binary E2E: no exact match within %d generations. Best fitness: %f (threshold: %f, max: %f)"), MaxGenerations, BestFitness, Threshold, MaxFitness);
			}
		}

		ASSERT_THAT(IsTrue(bMatched || BestFitness >= Threshold));
	}
};
