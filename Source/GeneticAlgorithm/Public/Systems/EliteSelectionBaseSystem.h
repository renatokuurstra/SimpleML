//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsContext.h"
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

	// Hook: return a human-readable label for a source entity (e.g. pawn actor name).
	// Derived systems in modules that know about vehicle components should override this.
	virtual FString GetSourceEntityLabel(entt::entity /*E*/) const { return TEXT("Unknown"); }

	// Hook: return the world location of a source entity.
	// Derived systems that can access location data (e.g. FVehicleComponent) should override this.
	virtual FVector GetSourceEntityLocation(entt::entity /*E*/) const { return FVector::ZeroVector; }

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
			
			// Add bounds checking for fitness array sizing
			if (FitnessIndex >= 0)
			{
				NewFit.Fitness.SetNum(FitnessIndex + 1, EAllowShrinking::No);
// Initialize with neutral fitness
			if (FitnessIndex >= 0 && FitnessIndex < NewFit.Fitness.Num())
			{
				NewFit.Fitness[FitnessIndex] = bHigherIsBetter ? -MAX_FLT : MAX_FLT;
			}
			}
			else
			{
				// Handle invalid fitness index
				NewFit.Fitness.SetNum(1, EAllowShrinking::No);
				NewFit.Fitness[0] = 0.0f;
			}
			
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
		
		// 1. Gather all candidates (eligible for breeding, non-elite, not flagged for reset)
		auto SourceView = Registry.view<FFitnessComponent, FUniqueSolutionComponent, FEligibleForBreedingTagComponent>(entt::exclude_t<FEliteTagComponent, FResetGenomeComponent>{});
		
		SelectionBuckets.Reset();
		
		for (auto Entity : SourceView)
		{
			const FFitnessComponent& Fit = SourceView.get<FFitnessComponent>(Entity);
			if (!IsCandidate(Entity, Fit)) { continue; }
			
			const int32 FitnessIndex = Fit.BuiltForFitnessIndex;
			if (FitnessIndex < 0) { continue; }

			if (SelectionBuckets.Num() <= FitnessIndex) { SelectionBuckets.SetNum(FitnessIndex + 1); }
			
			TArray<FEntityFitness>& Bucket = SelectionBuckets[FitnessIndex];
			FEntityFitness& Entry = Bucket.Emplace_GetRef();
			Entry.Value = (Fit.Fitness.Num() > FitnessIndex) ? Fit.Fitness[FitnessIndex] : 0.0f;
			Entry.Entity = Entity;
		}

		// 2. Identify existing elites and their SourceIds
		auto EliteView = Registry.view<FEliteTagComponent, FFitnessComponent, FUniqueSolutionComponent>();
		
		// Group existing elites by FitnessIndex
		TMap<int32, TArray<entt::entity>> ElitesByPop;
		int32 MaxPopIndex = -1;
		for (auto E : EliteView)
		{
			const FFitnessComponent& Fit = EliteView.get<FFitnessComponent>(E);
			ElitesByPop.FindOrAdd(Fit.BuiltForFitnessIndex).Add(E);
			MaxPopIndex = FMath::Max(MaxPopIndex, Fit.BuiltForFitnessIndex);
		}
		
		// Also find max pop index from candidates
		for (int32 i = 0; i < SelectionBuckets.Num(); ++i) { MaxPopIndex = FMath::Max(MaxPopIndex, i); }

		TMap<int32, float> TotalEliteFitnessPerPop;

		// 3. Process each population
		for (int32 FitnessIndex = 0; FitnessIndex <= MaxPopIndex; ++FitnessIndex)
		{
			TArray<entt::entity>& CurrentElites = ElitesByPop.FindOrAdd(FitnessIndex);
			TArray<FEntityFitness> EmptyBucket;
			TArray<FEntityFitness>& Candidates = SelectionBuckets.IsValidIndex(FitnessIndex) ? SelectionBuckets[FitnessIndex] : EmptyBucket;

			// Map SourceId -> Elite entity for existing elites in this population
			TMap<int64, entt::entity> SourceIdToElite;
			for (entt::entity EliteE : CurrentElites)
			{
				SourceIdToElite.Add(Registry.get<FUniqueSolutionComponent>(EliteE).SourceId, EliteE);
			}

			// Combine current elites and candidates into a pool of UNIQUE solutions (by ID)
			struct FUniqueCandidate
			{
				int64 Id = 0;
				float BestFitness = 0.0f;
				entt::entity SourceEntity = entt::null; // One of the entities with this ID
				entt::entity ExistingElite = entt::null;
			};
			TMap<int64, FUniqueCandidate> UniquePool;

			// Add existing elites to the pool
			for (entt::entity EliteE : CurrentElites)
			{
				int64 SId = Registry.get<FUniqueSolutionComponent>(EliteE).SourceId;
				FUniqueCandidate& UC = UniquePool.FindOrAdd(SId);
				UC.Id = SId;
				UC.BestFitness = Registry.get<FFitnessComponent>(EliteE).Fitness[FitnessIndex];
				UC.ExistingElite = EliteE;
			}

			// Add candidates to the pool
			for (const FEntityFitness& Cand : Candidates)
			{
				if (Cand.Entity == entt::null || !Registry.valid(Cand.Entity))
				{
					continue; // Skip invalid entities
				}
				
				int64 CId = Registry.get<FUniqueSolutionComponent>(Cand.Entity).Id;
				FUniqueCandidate* ExistingUC = UniquePool.Find(CId);
				
				// If this candidate's Id matches an existing elite's SourceId, the elite
				// is a snapshot of this solution at promotion time. We must NOT downgrade
				// the elite's fitness when the source entity has been reset or performed worse.
				// Only update if the candidate is genuinely better than the elite's recorded fitness.
				if (ExistingUC && ExistingUC->ExistingElite != entt::null)
				{
					// Candidate matches an existing elite by SourceId
					bool bCandidateIsBetter = bHigherIsBetter ? (Cand.Value > ExistingUC->BestFitness) : (Cand.Value < ExistingUC->BestFitness);
					if (!bCandidateIsBetter)
					{
						continue; // Don't downgrade elite fitness with a worse candidate
					}
					// Candidate is better - update the elite's fitness
					ExistingUC->BestFitness = Cand.Value;
					ExistingUC->SourceEntity = Cand.Entity;
					continue;
				}
				
				FUniqueCandidate& UC = UniquePool.FindOrAdd(CId);
				UC.Id = CId;
				if (UC.SourceEntity == entt::null || (bHigherIsBetter ? (Cand.Value > UC.BestFitness) : (Cand.Value < UC.BestFitness)))
				{
					UC.BestFitness = Cand.Value;
					UC.SourceEntity = Cand.Entity;
				}
			}

			// Sort unique pool by fitness
			TArray<FUniqueCandidate> SortedPool;
			UniquePool.GenerateValueArray(SortedPool);
			SortedPool.Sort([this](const FUniqueCandidate& A, const FUniqueCandidate& B)
			{
				if (bHigherIsBetter) return A.BestFitness > B.BestFitness;
				return A.BestFitness < B.BestFitness;
			});

			// Top N selection
			int32 ActualEliteCount = FMath::Min(EliteCount, SortedPool.Num());
			TArray<entt::entity> NewEliteEntities;
			float TotalEliteFitness = 0.0f;

			for (int32 i = 0; i < ActualEliteCount; ++i)
			{
				FUniqueCandidate& UC = SortedPool[i];
				TotalEliteFitness += UC.BestFitness;

				if (UC.ExistingElite != entt::null)
				{
					// Already an elite - check if fitness actually improved
					FFitnessComponent& Fit = Registry.get<FFitnessComponent>(UC.ExistingElite);
					const float OldFitness = Fit.Fitness[FitnessIndex];
					Fit.Fitness[FitnessIndex] = UC.BestFitness;
					Fit.EliteIndex = i;
					NewEliteEntities.Add(UC.ExistingElite);

					// Only update debug info and log when fitness genuinely improved
					const bool bFitnessImproved = bHigherIsBetter ? (UC.BestFitness > OldFitness) : (UC.BestFitness < OldFitness);
					if (bFitnessImproved && UC.SourceEntity != entt::null && Registry.valid(UC.SourceEntity))
					{
						FString SourceLabel = GetSourceEntityLabel(UC.SourceEntity);

						// Verbose logging for existing elite fitness update
						UE_LOG(LogTemp, Log, TEXT("[ELITE UPDATE] Pop %d | EliteRank %d | Source: %s | Fitness: %.2f -> %.2f | SourceId: %lld"),
							FitnessIndex, i, *SourceLabel, OldFitness, UC.BestFitness, UC.Id);

						// Update debug representation with location snapshot
						FElitePromotionDebugComponent& PromoDebug = Registry.get_or_emplace<FElitePromotionDebugComponent>(UC.ExistingElite);
						PromoDebug.Location = GetSourceEntityLocation(UC.SourceEntity);
						PromoDebug.Fitness = UC.BestFitness;
						PromoDebug.PopulationIndex = FitnessIndex;
						PromoDebug.ExpirationTime = GetContext()->GetWorld()->GetTimeSeconds() + 20.0f;
						PromoDebug.SourceEntity = UC.SourceEntity;
						PromoDebug.SourceLabel = SourceLabel;
					}
				}
				else
				{
// New solution became elite. We need to repurpose an old elite or create a new one.
					entt::entity TargetElite = entt::null;
					
					// Try to find an elite from CurrentElites that is NOT in the new top N
					for (int32 j = 0; j < CurrentElites.Num(); ++j)
					{
						// Add safety check for valid entity handle
						if (CurrentElites[j] != entt::null && 
							Registry.valid(CurrentElites[j]) && 
							!NewEliteEntities.Contains(CurrentElites[j]))
						{
							// Check if this elite we want to repurpose is needed later in SortedPool
							bool bNeededLater = false;
							for (int32 k = i + 1; k < ActualEliteCount; ++k)
							{
								if (SortedPool[k].ExistingElite != entt::null && 
									SortedPool[k].ExistingElite == CurrentElites[j]) 
								{ 
									bNeededLater = true; 
									break; 
								}
							}
							if (!bNeededLater)
							{
								TargetElite = CurrentElites[j];
								CurrentElites[j] = entt::null; // Mark as used
								break;
							}
						}
					}

if (TargetElite == entt::null)
		{
			TargetElite = Registry.create();
			Registry.emplace<FEliteTagComponent>(TargetElite);
			Registry.emplace<FUniqueSolutionComponent>(TargetElite);
			FFitnessComponent& NewFit = Registry.emplace<FFitnessComponent>(TargetElite);
			NewFit.BuiltForFitnessIndex = FitnessIndex;
			
			// Add bounds checking for fitness array sizing
			if (FitnessIndex >= 0)
			{
				NewFit.Fitness.SetNum(FitnessIndex + 1, EAllowShrinking::No);
			}
			else
			{
				// Handle invalid fitness index
				NewFit.Fitness.SetNum(1, EAllowShrinking::No);
			}
		}

// Update target elite
		Registry.get<FUniqueSolutionComponent>(TargetElite).SourceId = UC.Id;
		FFitnessComponent& Fit = Registry.get<FFitnessComponent>(TargetElite);
		Fit.Fitness[FitnessIndex] = UC.BestFitness;
		Fit.EliteIndex = i;
		
		CopyGenomeToElite(UC.SourceEntity, TargetElite, FitnessIndex);
		NewEliteEntities.Add(TargetElite);

		// Get source label for logging
		FString SourceLabel = GetSourceEntityLabel(UC.SourceEntity);

		// Verbose logging for new elite promotion
		UE_LOG(LogTemp, Log, TEXT("[ELITE PROMOTION] Pop %d | EliteRank %d | Source: %s | Fitness: %.2f | SourceId: %lld"),
			FitnessIndex, i, *SourceLabel, UC.BestFitness, UC.Id);

		// Add debug representation with location snapshot if source entity exists
		if (UC.SourceEntity != entt::null && Registry.valid(UC.SourceEntity))
		{
			FElitePromotionDebugComponent& PromoDebug = Registry.get_or_emplace<FElitePromotionDebugComponent>(TargetElite);
			PromoDebug.Location = GetSourceEntityLocation(UC.SourceEntity);
			PromoDebug.Fitness = UC.BestFitness;
			PromoDebug.PopulationIndex = FitnessIndex;
			PromoDebug.ExpirationTime = GetContext()->GetWorld()->GetTimeSeconds() + 20.0f;
			PromoDebug.SourceEntity = UC.SourceEntity;
			PromoDebug.SourceLabel = SourceLabel;
		}
				}
			}

			// Cleanup unused elites for this population
			for (entt::entity E : CurrentElites)
			{
				if (E != entt::null && !NewEliteEntities.Contains(E))
				{
					Registry.destroy(E);
				}
			}

			TotalEliteFitnessPerPop.Add(FitnessIndex, TotalEliteFitness);
		}

		// Update debug component
		auto DebugView = Registry.view<FGeneticAlgorithmDebugComponent>();
		for (auto DebugEntity : DebugView)
		{
			FGeneticAlgorithmDebugComponent& DebugComp = DebugView.get<FGeneticAlgorithmDebugComponent>(DebugEntity);
			for (int32 i = 0; i <= MaxPopIndex; ++i)
			{
				if (!TotalEliteFitnessPerPop.Contains(i)) { TotalEliteFitnessPerPop.Add(i, 0.0f); }
			}
			DebugComp.PopulationTotalEliteFitness = TotalEliteFitnessPerPop;
		}
	}
};
