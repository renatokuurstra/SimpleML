//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/EliteComponents.h"
#include "Components/GenomeComponents.h" // Forward reference not needed, but keep if required by UHT elsewhere
#include "EliteSelectionBaseSystem.generated.h"

/**
 * Abstract base for elite selection systems.
 * - Computes top-N per fitness index.
 * - Ensures per-index elite entity pools exist and are reused.
 * - Minimizes duplication by centralizing the selection loop and delegating type-specific save logic.
 *
 * TODO
 *  - Optimizations:
 *		- Move various arrays to member variables.
 *		- Ensure that if we already have the elite entity, we don't copy it over.
 *			- Right now, we copy NumElite * NumFitnesses every time we keep the elites.
 */
UCLASS(Abstract)
	class GENETICALGORITHM_API UEliteSelectionBaseSystem : public UEcsSystem
	{
		GENERATED_BODY()
		
	public:
  UEliteSelectionBaseSystem()
		{
			RegisterComponent<FFitnessComponent>();
			RegisterComponent<FEliteTagComponent>();
		}

		// Number of elites per fitness index to maintain. Treated as runtime parameter.
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Selection|Elite", meta=(ClampMin="1"))
		int32 EliteCount = 4;

		// If true, higher fitness is better.
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Selection|Elite")
		bool bHigherIsBetter = true;
		
		// Drive selection each tick from the base; children only implement hooks.
		virtual void Update(float DeltaTime = 0.0f) override
		{
			// UEcsSystem::Update is empty; directly invoke selection to avoid duplication in children.
			ApplySelection();
		}
		
		protected:
		// Hooks for type-specific behavior (implemented by derived systems)
		virtual bool IsCandidate(entt::entity E, const FFitnessComponent& Fit) const { return false; }
		virtual void CopyGenomeToElite(entt::entity Winner, entt::entity Elite, int32 FitnessIndex) {}

		// Internal pair used to track entity and its order for stable sorting.
		struct FEntityFitness
		{
			float Value = 0.0f;
			int32 Order = 0; // lower means earlier in the source stream (stable tie-break)
			entt::entity Entity = entt::null;
		};

		// Reusable caches to avoid per-tick allocations (kept as members and Reset each run)
		mutable TArray<TArray<FEntityFitness>> SelectionBuckets; // Per fitness index buckets
		mutable TArray<int32> IndexScratch; // reused for sorting indices

		// Compute top-N indices from a fitness array (stable tie-break by lower index)
		static void ComputeTopIndices(const TArray<float>& Fitness, int32 N, bool bHigher, TArray<int32>& OutIndices)
		{
			OutIndices.Reset(Fitness.Num());
			OutIndices.Reserve(Fitness.Num());
			for (int32 i = 0; i < Fitness.Num(); ++i) { OutIndices.Add(i); }
			OutIndices.Sort([&Fitness, bHigher](int32 A, int32 B)
			{
				if (bHigher)
				{
					if (Fitness[A] != Fitness[B]) return Fitness[A] > Fitness[B];
				}
				else
				{
					if (Fitness[A] != Fitness[B]) return Fitness[A] < Fitness[B];
				}
				return A < B;
			});
			if (OutIndices.Num() > N) { OutIndices.SetNum(N, EAllowShrinking::No); }
		}


		// Helper: collect or create elite entities for a given fitness index.
		// Reuses existing elites with FFitnessComponent.BuiltForFitnessIndex == FitnessIndex.
		inline void GatherElitePool(int32 FitnessIndex, int32 DesiredCount, TArray<entt::entity>& OutEntities)
		{
			auto& Registry = GetRegistry();
			OutEntities.Reset();
			// Reuse existing elites for this index
			auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent>();
			for (auto E : EliteView)
			{
				const FFitnessComponent& Fit = EliteView.get<FFitnessComponent>(E);
				if (Fit.BuiltForFitnessIndex == FitnessIndex)
				{
					OutEntities.Add(E);
				}
			}
			// Create missing elites
			while (OutEntities.Num() < DesiredCount)
			{
				const entt::entity NewE = Registry.create();
				Registry.emplace<FEliteTagComponent>(NewE, FEliteTagComponent{});
				FFitnessComponent NewFit{};
				NewFit.BuiltForFitnessIndex = FitnessIndex;
				NewFit.Fitness.SetNum(FitnessIndex + 1, EAllowShrinking::No);
				Registry.emplace<FFitnessComponent>(NewE, MoveTemp(NewFit));
				OutEntities.Add(NewE);
			}
			// Remove extras
			if (OutEntities.Num() > DesiredCount)
			{
				for (int32 i = DesiredCount; i < OutEntities.Num(); ++i)
				{
					Registry.destroy(OutEntities[i]);
				}
				OutEntities.SetNum(DesiredCount, EAllowShrinking::No);
			}
		}

		// Centralized selection flow: compute top-N per fitness index and materialize elites as separate entities.
		// Type-specific genome copying/binding is delegated to virtual hooks implemented by derived systems.
		void ApplySelection()
		{
			auto& Registry = GetRegistry();
			// Source: all non-elite entities that have fitness; derived systems decide if an entity is a valid candidate via IsCandidate
			auto SourceView = Registry.view<FFitnessComponent>(entt::exclude_t<FEliteTagComponent>{});
		
			SelectionBuckets.Reset();
			IndexScratch.Reset();
		
			// Stream all candidates into per-index buckets
			int32 CurrentOrder = 0;
			for (auto Entity : SourceView)
			{
				const FFitnessComponent& Fit = SourceView.get<FFitnessComponent>(Entity);
				if (!IsCandidate(Entity, Fit)) { ++CurrentOrder; continue; }
				const int32 Dims = Fit.Fitness.Num();
				if (SelectionBuckets.Num() < Dims) { SelectionBuckets.SetNum(Dims); }
				for (int32 idx = 0; idx < Dims; ++idx)
				{
					TArray<FEntityFitness>& Bucket = SelectionBuckets[idx];
					FEntityFitness& Entry = Bucket.Emplace_GetRef();
					Entry.Value = Fit.Fitness[idx];
					Entry.Order = CurrentOrder;
					Entry.Entity = Entity;
				}
				++CurrentOrder;
			}
		
			// For each fitness index, pick top-N and copy into elite-owned storage
			for (int32 FitnessIndex = 0; FitnessIndex < SelectionBuckets.Num(); ++FitnessIndex)
			{
				TArray<FEntityFitness>& Bucket = SelectionBuckets[FitnessIndex];
				if (Bucket.Num() == 0) { continue; }
				const int32 N = FMath::Clamp(EliteCount, 1, Bucket.Num());
				IndexScratch.Reset(Bucket.Num());
				for (int32 i = 0; i < Bucket.Num(); ++i) { IndexScratch.Add(i); }
				IndexScratch.Sort([this, &Bucket](int32 A, int32 B)
				{
					const float VA = Bucket[A].Value;
					const float VB = Bucket[B].Value;
					if (bHigherIsBetter)
					{
						if (VA != VB) return VA > VB;
					}
					else
					{
						if (VA != VB) return VA < VB;
					}
					return Bucket[A].Order < Bucket[B].Order;
				});
				if (IndexScratch.Num() > N) { IndexScratch.SetNum(N, EAllowShrinking::No); }
				
				// Ensure/reuse elite entities for this index
				TArray<entt::entity> ElitePool;
				GatherElitePool(FitnessIndex, N, ElitePool);
				// Copy top winners into elite pool
				for (int32 r = 0; r < N; ++r)
				{
					const FEntityFitness& Winner = Bucket[IndexScratch[r]];
					const entt::entity EliteE = ElitePool[r];
					// Ensure elite has correct fitness component sizing and value
					FFitnessComponent& EliteFit = Registry.get<FFitnessComponent>(EliteE);
					if (EliteFit.Fitness.Num() < FitnessIndex + 1)
					{
						EliteFit.Fitness.SetNum(FitnessIndex + 1, EAllowShrinking::No);
					}
					EliteFit.Fitness[FitnessIndex] = Winner.Value;
					EliteFit.BuiltForFitnessIndex = FitnessIndex;
					
					// Delegate type-specific genome copy/bind
					CopyGenomeToElite(Winner.Entity, EliteE, FitnessIndex);
				}
			}
		}
	};
