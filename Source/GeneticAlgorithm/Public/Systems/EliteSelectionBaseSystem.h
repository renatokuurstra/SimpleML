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
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class GENETICALGORITHM_API UEliteSelectionBaseSystem : public UEcsSystem
	{
		GENERATED_BODY()
		
	public:
		UEliteSelectionBaseSystem();

		// Number of elites per fitness index to maintain. Treated as runtime parameter.
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Selection|Elite", meta=(ClampMin="1"))
		int32 EliteCount = 4;

		// If true, higher fitness is better.
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Selection|Elite")
		bool bHigherIsBetter = true;
		
		// Drive selection each tick from the base; children only implement hooks.
		virtual void Update_Implementation(float DeltaTime) override;
		
 	protected:
		// Hooks for type-specific behavior (implemented by derived systems)
		virtual bool IsCandidate(entt::entity E, const FFitnessComponent& Fit) const { return true; }
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

			// Sort existing elites so that when we compare them against candidates, 
			// we can reliably replace the worst ones if needed.
			OutEntities.Sort([this, &Registry, FitnessIndex](entt::entity A, entt::entity B)
			{
				const float VA = Registry.get<FFitnessComponent>(A).Fitness[FitnessIndex];
				const float VB = Registry.get<FFitnessComponent>(B).Fitness[FitnessIndex];
				if (bHigherIsBetter) { return VA > VB; }
				return VA < VB;
			});

			// Create missing elites
			while (OutEntities.Num() < DesiredCount)
			{
				const entt::entity NewE = Registry.create();
				Registry.emplace<FEliteTagComponent>(NewE, FEliteTagComponent{});
				FFitnessComponent NewFit{};
				NewFit.BuiltForFitnessIndex = FitnessIndex;
				NewFit.Fitness.SetNum(FitnessIndex + 1, EAllowShrinking::No);
				// Initialize with neutral fitness
				NewFit.Fitness[FitnessIndex] = bHigherIsBetter ? -MAX_FLT : MAX_FLT;
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
			// We only consider entities that are eligible for breeding.
			auto SourceView = Registry.view<FFitnessComponent, FEligibleForBreedingTagComponent>(entt::exclude_t<FEliteTagComponent>{});
		
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
		
			// For each fitness index, maintain and update elites
			for (int32 FitnessIndex = 0; FitnessIndex < SelectionBuckets.Num(); ++FitnessIndex)
			{
				TArray<FEntityFitness>& Bucket = SelectionBuckets[FitnessIndex];
				
				// Ensure we have the desired number of elite entities for this index
				TArray<entt::entity> ElitePool;
				GatherElitePool(FitnessIndex, EliteCount, ElitePool);
				
				if (Bucket.Num() == 0) { continue; }

				// Sort candidates
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

				// 1. Combine current elites and candidates into a single pool.
				struct FPotentialElite
				{
					float Fitness = 0.0f;
					entt::entity SourceEntity = entt::null; // null if it's already an elite
					entt::entity ExistingEliteEntity = entt::null; // if it's already an elite
				};

				TArray<FPotentialElite> PotentialElites;
				PotentialElites.Reserve(ElitePool.Num() + Bucket.Num());

				// Add current elites to potential pool
				for (entt::entity EliteE : ElitePool)
				{
					FPotentialElite PE;
					PE.Fitness = Registry.get<FFitnessComponent>(EliteE).Fitness[FitnessIndex];
					PE.ExistingEliteEntity = EliteE;
					PotentialElites.Add(PE);
				}

				// Add candidates to potential pool
				for (int32 i = 0; i < IndexScratch.Num(); ++i)
				{
					const FEntityFitness& Candidate = Bucket[IndexScratch[i]];
					FPotentialElite PE;
					PE.Fitness = Candidate.Value;
					PE.SourceEntity = Candidate.Entity;
					PotentialElites.Add(PE);
				}

				// 2. Sort the potential pool
				PotentialElites.Sort([this](const FPotentialElite& A, const FPotentialElite& B)
				{
					if (bHigherIsBetter)
					{
						return A.Fitness > B.Fitness;
					}
					return A.Fitness < B.Fitness;
				});

				// 3. Take top N and update ElitePool
				TArray<entt::entity> NewEliteEntities;
				NewEliteEntities.Reserve(EliteCount);
				
				TArray<int32> WinningPEIndices;
				WinningPEIndices.Reserve(EliteCount);

				int32 PEIdx = 0;
				while (WinningPEIndices.Num() < EliteCount && PEIdx < PotentialElites.Num())
				{
					WinningPEIndices.Add(PEIdx);
					if (PotentialElites[PEIdx].ExistingEliteEntity != entt::null)
					{
						NewEliteEntities.Add(PotentialElites[PEIdx].ExistingEliteEntity);
					}
					PEIdx++;
				}

				// Repurposing logic:
				TArray<entt::entity> ElitesToRepurpose;
				for (entt::entity E : ElitePool)
				{
					if (!NewEliteEntities.Contains(E))
					{
						ElitesToRepurpose.Add(E);
					}
				}

				int32 RepurposeIdx = 0;
				for (int32 i = 0; i < WinningPEIndices.Num(); ++i)
				{
					int32 WinnerIdx = WinningPEIndices[i];
					FPotentialElite& PE = PotentialElites[WinnerIdx];
					
					if (PE.ExistingEliteEntity != entt::null)
					{
						Registry.get<FFitnessComponent>(PE.ExistingEliteEntity).EliteIndex = i;
					}
					else
					{
						entt::entity TargetElite = ElitesToRepurpose[RepurposeIdx++];
						FFitnessComponent& EliteFit = Registry.get<FFitnessComponent>(TargetElite);
						EliteFit.Fitness[FitnessIndex] = PE.Fitness;
						EliteFit.BuiltForFitnessIndex = FitnessIndex;
						EliteFit.EliteIndex = i;
						CopyGenomeToElite(PE.SourceEntity, TargetElite, FitnessIndex);
					}
				}
			}
		}
	};
