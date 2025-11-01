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
	int32 MaxGenerations = 500;
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
		const FString TargetString = TEXT("This is an integration test for GA binary data");
		// Convert wide string to UTF-8 bytes; our genome is byte-based and uses ASCII here.
		{
			FTCHARToUTF8 Converter(*TargetString);
			GenomeLen = Converter.Length();
			TargetBytes.Reset(GenomeLen);
			TargetBytes.AddUninitialized(GenomeLen);
			FMemory::Memcpy(TargetBytes.GetData(), (const void*)Converter.Get(), GenomeLen);
		}
		PopulationSize = 50;
		MaxGenerations = 500;
		Seed = 1337;

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
		Selection->TournamentSize = 5;
		Selection->SelectionPressure = 0.9f;
		Selection->bHigherIsBetter = true;
		Selection->CrossGroupParentChance = 0.0f; // single group
		Selection->RandomSeed = Seed + 1;

		Breeder = NewObject<UBreedCharGenomesSystem>();
		Breeder->Initialize(Registry);
		Breeder->RandomSeed = Seed + 2;

		Mutator = NewObject<UMutationCharGenomeSystem>();
		Mutator->Initialize(Registry);
		Mutator->BitFlipProbability = 0.01f; // 1% per-bit as requested
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
		int32 MatchedGeneration = -1;
		TArray<char> BestSoFar;
		BestSoFar.SetNum(GenomeLen, EAllowShrinking::No);

		for (int32 Gen = 0; Gen < MaxGenerations; ++Gen)
		{
			// 1) Evaluate fitness
			GATestHelper::ComputeBinaryFitness(Registry, TargetBytes);

			// 1b) Mark all entities as reset candidates for next generation
			{
				auto PopView = Registry.view<FFitnessComponent>();
				for (auto E : PopView)
				{
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
				// Periodic debug log of best candidate every 10 generations
				if ((Gen % 10) == 0)
				{
					// Convert current best bytes (UTF-8) into a temporary null-terminated buffer for logging
					TArray<ANSICHAR> Tmp;
					Tmp.SetNumUninitialized(GenomeLen + 1);
					FMemory::Memcpy(Tmp.GetData(), BestSoFar.GetData(), GenomeLen);
					Tmp[GenomeLen] = '\0';
					FString BestStr = UTF8_TO_TCHAR(Tmp.GetData());
					UE_LOG(LogTemp, Display, TEXT("[GA Binary Test] Gen=%d BestFitness=%f Best=\"%s\""), Gen, BestFitnessLocal, *BestStr);
				}
				if (bEqual)
				{
					bMatched = true;
					MatchedGeneration = Gen;
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
			UE_LOG(LogTemp, Warning, TEXT("GA binary E2E: no exact match within %d generations. Best fitness: %f (threshold: %f, max: %f)"), MaxGenerations, BestFitness, Threshold, MaxFitness);
		}

		ASSERT_THAT(IsTrue(bMatched || BestFitness >= Threshold));
	}
};
