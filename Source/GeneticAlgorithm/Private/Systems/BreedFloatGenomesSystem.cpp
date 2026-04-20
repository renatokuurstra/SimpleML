//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/BreedFloatGenomesSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/BreedingPairComponent.h"
#include "Math/UnrealMathUtility.h"

float UBreedFloatGenomesSystem::SampleSbxChild(float X1, float X2, float U, float EtaLocal, bool bPickFirst) const
{
	// Ensure order for formula symmetry
	float A = X1;
	float B = X2;
	if (A > B)
	{
		Swap(A, B);
	}

	// Deb's SBX: compute beta_q from uniform U in [0,1]
	const float OneOver = 1.0f / (EtaLocal + 1.0f);
	float BetaQ;
	if (U <= 0.5f)
	{
		BetaQ = FMath::Pow(2.0f * U, OneOver);
	}
	else
	{
		BetaQ = FMath::Pow(1.0f / (2.0f * (1.0f - U)), OneOver);
	}

	// Two children (child1 near A, child2 near B)
	const float Child1 = 0.5f * ((A + B) - BetaQ * (B - A));
	const float Child2 = 0.5f * ((A + B) + BetaQ * (B - A));

	// Map back to original parent order
	if (X1 <= X2)
	{
		return bPickFirst ? Child1 : Child2;
	}
	else
	{
		return bPickFirst ? Child2 : Child1;
	}
}

void UBreedFloatGenomesSystem::Update_Implementation(float /*DeltaTime*/)
{
	auto& Registry = GetRegistry();

	auto ResetView = GetView<FResetGenomeComponent, FGenomeFloatViewComponent>();
	auto PairView = GetView<FBreedingPairComponent>();

	if (ResetView.begin() == ResetView.end())
	{
		return; // nothing to reset
	}

	// Build a map from child entity id -> breeding pair, keyed by ChildEntity.
	// Only pairs that name a specific child (ChildEntity != 0) are usable.
	TMap<uint32, const FBreedingPairComponent*> PairByChild;
	for (auto PairEntity : PairView)
	{
		const FBreedingPairComponent& Pair = Registry.get<FBreedingPairComponent>(PairEntity);
		if (Pair.ChildEntity != 0u)
		{
			PairByChild.Add(Pair.ChildEntity, &Pair);
		}
	}

	if (PairByChild.IsEmpty())
	{
		return; // no eligible children to breed this tick
	}

	// RNG
	FRandomStream Rng;
	FRandomStream* RngPtr = nullptr;
	if (RandomSeed != 0)
	{
		Rng.Initialize(RandomSeed);
		RngPtr = &Rng;
	}

	int32 Index = 0;
	for (auto ResetIt = ResetView.begin(), ResetEnd = ResetView.end(); ResetIt != ResetEnd; ++ResetIt, ++Index)
	{
		const entt::entity ChildEntity = *ResetIt;
		const uint32 ChildId = static_cast<uint32>(ChildEntity);

		// Only breed this child if tournament selection produced a pair for it
		const FBreedingPairComponent* const* FoundPair = PairByChild.Find(ChildId);
		if (!FoundPair)
		{
			continue; // not eligible for breeding this tick (e.g. too young, blocked reason)
		}
		const FBreedingPairComponent& Pair = **FoundPair;

		const entt::entity ParentA = static_cast<entt::entity>(Pair.ParentA);
		const entt::entity ParentB = static_cast<entt::entity>(Pair.ParentB);

		if (ParentA == entt::null || ParentB == entt::null)
		{
			UE_LOG(LogTemp, Warning, TEXT("BreedFloatGenomesSystem: invalid parent(s) in pair at reset index %d"), Index);
			continue;
		}

		// Resolve parent genome views
		if (!Registry.all_of<FGenomeFloatViewComponent>(ChildEntity))
		{
			UE_LOG(LogTemp, Warning, TEXT("BreedFloatGenomesSystem: missing FGenomeFloatViewComponent on child (reset index=%d)"), Index);
			continue;
		}

		TOptional<TArrayView<const float>> MaybeA;
		TOptional<TArrayView<const float>> MaybeB;

		if (Registry.all_of<FGenomeFloatViewComponent>(ParentA))
		{
			const FGenomeFloatViewComponent& AViewComp = Registry.get<FGenomeFloatViewComponent>(ParentA);
			MaybeA.Emplace(TArrayView<const float>(AViewComp.Values.GetData(), AViewComp.Values.Num()));
		}

		if (Registry.all_of<FGenomeFloatViewComponent>(ParentB))
		{
			const FGenomeFloatViewComponent& BViewComp = Registry.get<FGenomeFloatViewComponent>(ParentB);
			MaybeB.Emplace(TArrayView<const float>(BViewComp.Values.GetData(), BViewComp.Values.Num()));
		}

		if (!MaybeA.IsSet() || !MaybeB.IsSet())
		{
			UE_LOG(LogTemp, Warning, TEXT("BreedFloatGenomesSystem: missing genome on parent(s) (reset index=%d)"), Index);
			continue;
		}

		FGenomeFloatViewComponent& ChildViewComp = Registry.get<FGenomeFloatViewComponent>(ChildEntity);
		TArrayView<const float> AView = MaybeA.GetValue();
		TArrayView<const float> BView = MaybeB.GetValue();
		TArrayView<float> CView = ChildViewComp.Values;

		const int32 GeneCount = FMath::Min3(AView.Num(), BView.Num(), CView.Num());
		if (GeneCount <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("BreedFloatGenomesSystem: zero-length genome at reset index=%d"), Index);
			continue;
		}

		for (int32 g = 0; g < GeneCount; ++g)
		{
			const float a = AView[g];
			const float b = BView[g];
			const float rand01 = RngPtr ? RngPtr->FRand() : FMath::FRand();
			float value;
			if (rand01 < CrossoverProbability)
			{
				const float u = RngPtr ? RngPtr->FRand() : FMath::FRand();
				const bool bFirst = (RngPtr ? (RngPtr->RandRange(0,1) == 0) : (FMath::RandRange(0,1) == 0));
				value = SampleSbxChild(a, b, u, Eta, bFirst);
			}
			else
			{
				// Copy a gene from a random parent when no crossover
				const bool bPickA = (RngPtr ? (RngPtr->RandRange(0,1) == 0) : (FMath::RandRange(0,1) == 0));
				value = bPickA ? a : b;
			}

			if (bClampChildren)
			{
				value = FMath::Clamp(value, ClampMin, ClampMax);
			}
			CView[g] = value;
		}

		// Reset child's fitness now that a new genome is installed
		if (Registry.all_of<FFitnessComponent>(ChildEntity))
		{
			FFitnessComponent& Fit = Registry.get<FFitnessComponent>(ChildEntity);
			if (Fit.Fitness.Num() == 0) { Fit.Fitness.SetNum(1, EAllowShrinking::No); }
			for (int32 iFit = 0; iFit < Fit.Fitness.Num(); ++iFit) { Fit.Fitness[iFit] = 0.0f; }
		}

		// Assign a new unique ID since this is a new solution
		if (Registry.all_of<FUniqueSolutionComponent>(ChildEntity))
		{
			Registry.get<FUniqueSolutionComponent>(ChildEntity).Id = FUniqueSolutionComponent::GenerateNewId();
		}
		else
		{
			Registry.emplace<FUniqueSolutionComponent>(ChildEntity).Id = FUniqueSolutionComponent::GenerateNewId();
		}
	}
}
