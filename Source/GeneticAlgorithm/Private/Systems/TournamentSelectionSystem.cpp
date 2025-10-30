//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/TournamentSelectionSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/BreedingPairComponent.h"
#include "Math/UnrealMathUtility.h"

entt::entity UTournamentSelectionSystem::RunTournament(const TArray<FEntityRefFitness>& Bucket, FRandomStream* Rng)
{
	const int32 Size = Bucket.Num();
	if (Size <= 0)
	{
		return entt::null;
	}
	const int32 K = FMath::Clamp(TournamentSize, 2, Size);

	// Sample K indices into ScratchIndices
	ScratchIndices.Reset();
	if (bWithReplacement)
	{
		ScratchIndices.Reserve(K);
		for (int32 i = 0; i < K; ++i)
		{
			const int32 Pick = (Rng ? Rng->RandRange(0, Size - 1) : FMath::RandRange(0, Size - 1));
			ScratchIndices.Add(Pick);
		}
	}
	else
	{
		// Reservoir-like shuffle for unique picks when K << Size
		ScratchIndices.Reserve(Size);
		for (int32 i = 0; i < Size; ++i) { ScratchIndices.Add(i); }
		// Partial Fisher-Yates up to K
		for (int32 i = 0; i < K; ++i)
		{
			const int32 SwapIdx = i + (Rng ? Rng->RandRange(0, Size - 1 - i) : FMath::RandRange(0, Size - 1 - i));
			ScratchIndices.Swap(i, SwapIdx);
		}
		ScratchIndices.SetNum(K, EAllowShrinking::No);
	}

	// Find best and second best in sampled candidates
	int32 Best = INDEX_NONE;
	int32 Second = INDEX_NONE;
	auto Better = [this, &Bucket](int32 A, int32 B)
	{
		if (A == INDEX_NONE) return false;
		if (B == INDEX_NONE) return true;
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
	};
	for (int32 i = 0; i < ScratchIndices.Num(); ++i)
	{
		const int32 idx = ScratchIndices[i];
		if (Best == INDEX_NONE || Better(idx, Best))
		{
			Second = Best;
			Best = idx;
		}
		else if (Second == INDEX_NONE || Better(idx, Second))
		{
			Second = idx;
		}
	}

	// Apply selection pressure
	const bool PickBest = (SelectionPressure >= 1.0f) || ((Rng ? Rng->FRand() : FMath::FRand()) <= SelectionPressure);
	const int32 WinnerIdx = PickBest || Second == INDEX_NONE ? Best : Second;
	return WinnerIdx != INDEX_NONE ? Bucket[WinnerIdx].Entity : entt::null;
}

void UTournamentSelectionSystem::Update(float /*DeltaTime*/)
{
	auto& Registry = GetRegistry();

	// Build reset target list
	ResetTargets.Reset();
	{
		auto ResetView = GetView<FFitnessComponent, FResetGenomeComponent>();
		for (auto Entity : ResetView)
		{
			ResetTargets.Add(Entity);
		}
	}
	if (ResetTargets.Num() == 0)
	{
		return; // Nothing to do
	}

	// Stream all candidates and bucket them by BuiltForFitnessIndex (and globally)
	GroupBuckets.Reset();
	GlobalBucket.Reset();
	int32 Order = 0;
	{
		auto PopView = GetView<FFitnessComponent>();
		for (auto Entity : PopView)
		{
			const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(Entity);
			const int32 Dims = Fit.Fitness.Num();
			if (Dims <= 0) { ++Order; continue; }
			if (GroupBuckets.Num() < Dims) { GroupBuckets.SetNum(Dims); }

			// Push to each group bucket according to its own index? We group by entity's BuiltForFitnessIndex
			const int32 Group = Fit.BuiltForFitnessIndex;
			if (Group >= 0 && Group < Dims)
			{
				TArray<FEntityRefFitness>& Bucket = GroupBuckets[Group];
				FEntityRefFitness& E = Bucket.Emplace_GetRef();
				E.Value = Fit.Fitness[Group];
				E.Order = Order;
				E.Entity = Entity;
			}
			// Also push to global using the same representative value when valid; otherwise skip global
			if (Group >= 0 && Group < Dims)
			{
				FEntityRefFitness& G = GlobalBucket.Emplace_GetRef();
				G.Value = Fit.Fitness[Group];
				G.Order = Order;
				G.Entity = Entity;
			}
			++Order;
		}
	}

	// RNG setup
	FRandomStream Rng;
	FRandomStream* RngPtr = nullptr;
	if (RandomSeed != 0)
	{
		Rng.Initialize(RandomSeed);
		RngPtr = &Rng;
	}

	// For each reset target, pick two parents according to group preference and cross-group chance
	for (entt::entity Target : ResetTargets)
	{
		const FFitnessComponent& TargetFit = Registry.get<FFitnessComponent>(Target);
		const int32 Group = TargetFit.BuiltForFitnessIndex;
		const bool bValidGroup = Group >= 0 && Group < GroupBuckets.Num() && GroupBuckets[Group].Num() > 0;
		const TArray<FEntityRefFitness>* Preferred = bValidGroup ? &GroupBuckets[Group] : nullptr;
		const bool bHasGlobal = GlobalBucket.Num() > 0;

		if (!Preferred && !bHasGlobal)
		{
			continue; // No candidates
		}
		entt::entity Parents[2] = { entt::null, entt::null };

		for (int ParentIdx = 0; ParentIdx < 2; ++ParentIdx)
		{
			const bool bUseGlobal = bHasGlobal && ((RngPtr ? RngPtr->FRand() : FMath::FRand()) < CrossGroupParentChance);
			const TArray<FEntityRefFitness>& Bucket = (bUseGlobal || !Preferred) ? GlobalBucket : *Preferred;
			Parents[ParentIdx] = RunTournament(Bucket, RngPtr);
		}

		// If we have two valid parents, create a child and emit a linkage entity with FParentsChildComponent
		if (Parents[0] != entt::null && Parents[1] != entt::null)
		{
			const entt::entity LinkEntity = Registry.create();
			FBreedingPairComponent Link{};
			Link.ParentA = static_cast<uint32>(Parents[0]);
			Link.ParentB = static_cast<uint32>(Parents[1]);
			Registry.emplace<FBreedingPairComponent>(LinkEntity, Link);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("TournamentSelectionSystem: no valid parents for target %d."), Target);
		}
	}
}
