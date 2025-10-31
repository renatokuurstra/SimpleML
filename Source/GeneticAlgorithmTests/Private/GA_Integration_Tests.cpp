// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
// GA Integration Tests file: end-to-end CQTest for binary (char) genomes.

#include "CoreMinimal.h"
#include "CQTest.h"

#include "entt/entt.hpp"

// GA components and systems
#include "Components/GenomeComponents.h"
#include "Components/BreedingPairComponent.h"
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
		static const char* TargetLiteral = "This is an integration test for GA binary data";
		GenomeLen = static_cast<int32>(FCStringAnsi::Strlen(TargetLiteral));
		PopulationSize = 50;
		MaxGenerations = 500;
		Seed = 1337;

		// Convert target to bytes
		TargetBytes.Reset();
		TargetBytes.Reserve(GenomeLen);
		for (int32 i = 0; i < GenomeLen; ++i)
		{
			TargetBytes.Add(static_cast<uint8>(TargetLiteral[i]));
		}

		// Prepare genome storage
		Genomes.SetNum(PopulationSize, EAllowShrinking::No);

		// Create population without keeping an entity array; bind components and randomize genomes
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
			Registry.emplace<FResetGenomeComponent>(E, FResetGenomeComponent{});

			GATestHelper::InitializeRandomGenome(Genomes[i], Registry.get<FGenomeCharViewComponent>(E), GenomeLen, Rng);
		}

		// Systems
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
				if (bEqual)
				{
					bMatched = true;
					MatchedGeneration = Gen;
					break;
				}
			}

			// 3) Selection → breeding → mutation → cleanup
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
