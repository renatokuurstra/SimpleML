#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
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
			RegisterComponent<FEliteGroupIndex>();
		}

		// Number of elites per fitness index to maintain. Treated as runtime parameter.
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Selection|Elite", meta=(ClampMin="1"))
		int32 EliteCount = 4;

		// If true, higher fitness is better.
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeneticAlgorithm|Selection|Elite")
		bool bHigherIsBetter = true;

	protected:
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

		// Derived must copy the genome data from the source entity to the elite entity
		virtual void SaveEliteGenome(entt::entity /*SourceEntity*/, entt::entity /*EliteEntity*/)
		{
			// Base does nothing; derived classes must override.
		}

		// Helper: Ensure there are at least DesiredCount elite entities for the given fitness index and component type.
		template<typename TEliteComponent>
		void EnsureEliteEntities(int32 FitnessIndex, int32 DesiredCount, TArray<entt::entity>& OutEntities)
		{
			OutEntities.Reset();
			auto& Registry = GetRegistry();
			auto View = Registry.view<FEliteGroupIndex, TEliteComponent>();
			for (auto Entity : View)
			{
				auto& Group = View.get<FEliteGroupIndex>(Entity);
				if (Group.FitnessIndex == FitnessIndex)
				{
					OutEntities.Add(Entity);
					if (OutEntities.Num() >= DesiredCount) { break; }
				}
			}
			while (OutEntities.Num() < DesiredCount)
			{
				entt::entity E = Registry.create();
				Registry.emplace<FEliteGroupIndex>(E, FEliteGroupIndex{FitnessIndex});
				Registry.emplace<TEliteComponent>(E, TEliteComponent{});
				OutEntities.Add(E);
			}
		}

		// Centralized, templated selection flow for a given genome view and elite component type.
		template<typename TGenomeViewComponent, typename TEliteComponent>
		void ApplySelectionFor()
		{
			// Source population size = number of entities with the genome view component
			auto SourceView = GetView<TGenomeViewComponent, FFitnessComponent>();

			// Reset reusable caches
			SelectionBuckets.Reset();
			IndexScratch.Reset();

			int32 Population = 0;
			
			// Stream once through the source view and bucket by fitness index
			int32 CurrentOrder = 0;
			for (auto SourceEntity : SourceView)
			{
				Population++;
				auto& Registry = GetRegistry();

				const FFitnessComponent& FitnessComp = Registry.get<FFitnessComponent>(SourceEntity);
				const int32 FitnessDims = FitnessComp.Fitness.Num();
				
				if (SelectionBuckets.Num() < FitnessDims)
				{
					SelectionBuckets.SetNum(FitnessDims);
				}
				for (int32 idx = 0; idx < FitnessDims; ++idx)
				{
					TArray<FEntityFitness>& Bucket = SelectionBuckets[idx];
					FEntityFitness& Entry = Bucket.Emplace_GetRef();
					Entry.Value = FitnessComp.Fitness[idx];
					Entry.Order = CurrentOrder;
					Entry.Entity = SourceEntity;
				}
				++CurrentOrder;
			}

			// For each fitness index, select elites and copy genomes
			for (int32 FitnessIndex = 0; FitnessIndex < SelectionBuckets.Num(); ++FitnessIndex)
			{
				TArray<FEntityFitness>& Bucket = SelectionBuckets[FitnessIndex];
				if (Bucket.Num() == 0)
				{
					continue;
				}

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
					return Bucket[A].Order < Bucket[B].Order; // stable tie-break by earlier order
				});
				if (IndexScratch.Num() > N) { IndexScratch.SetNum(N, EAllowShrinking::No); }

				TArray<entt::entity> EliteEntities;
				EnsureEliteEntities<TEliteComponent>(FitnessIndex, EliteCount, EliteEntities);

				const int32 CopyCount = FMath::Min3(IndexScratch.Num(), Population, EliteEntities.Num());
				for (int32 Rank = 0; Rank < CopyCount; ++Rank)
				{
					const entt::entity Src = Bucket[IndexScratch[Rank]].Entity;
					const entt::entity DstElite = EliteEntities[Rank];
					SaveEliteGenome(Src, DstElite);
				}
				// Remaining elite entities (if any) are intentionally left unchanged.
			}
		}
	};
