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

void UBreedFloatGenomesSystem::Update(float /*DeltaTime*/)
{
	auto& Registry = GetRegistry();

	// Views: processed in-order via iterators (no caching)
	auto ResetView = GetView<FResetGenomeComponent, FGenomeFloatViewComponent>();
	auto PairView = GetView<FBreedingPairComponent>();

	if (ResetView.begin() == ResetView.end())
	{
		return; // nothing to reset
	}
	if (PairView.begin() == PairView.end())
	{
		UE_LOG(LogTemp, Warning, TEXT("BreedFloatGenomesSystem: no FBreedingPairComponent entities available."));
		return;
	}

	// RNG
	FRandomStream Rng;
	FRandomStream* RngPtr = nullptr;
	if (RandomSeed != 0)
	{
		Rng.Initialize(RandomSeed);
		RngPtr = &Rng;
	}

	auto PairIt = PairView.begin();
	const auto PairEnd = PairView.end();
	int32 Index = 0;

	for (auto ResetIt = ResetView.begin(), ResetEnd = ResetView.end(); ResetIt != ResetEnd; ++ResetIt, ++Index)
	{
		if (PairIt == PairEnd)
		{
			UE_LOG(LogTemp, Warning, TEXT("BreedFloatGenomesSystem: ran out of FBreedingPairComponent entities after processing %d reset entities."), Index);
			break;
		}

		const entt::entity ChildEntity = *ResetIt;
		const entt::entity PairEntity = *PairIt;
		++PairIt; // consume one pair per reset entity

		const FBreedingPairComponent& Pair = Registry.get<FBreedingPairComponent>(PairEntity);
		const entt::entity ParentA = static_cast<entt::entity>(Pair.ParentA);
		const entt::entity ParentB = static_cast<entt::entity>(Pair.ParentB);

		if (ParentA == entt::null || ParentB == entt::null)
		{
			UE_LOG(LogTemp, Warning, TEXT("BreedFloatGenomesSystem: invalid parent(s) in pair at reset index %d"), Index);
			continue;
		}

		//TODO: Bit suspicious, maybe there is a better way to check this?
		// Validate components
		if (!Registry.all_of<FGenomeFloatViewComponent>(ParentA) || !Registry.all_of<FGenomeFloatViewComponent>(ParentB) || !Registry.all_of<FGenomeFloatViewComponent>(ChildEntity))
		{
			UE_LOG(LogTemp, Warning, TEXT("BreedFloatGenomesSystem: missing FGenomeFloatViewComponent on parent or child (reset index=%d)"), Index);
			continue;
		}

		const FGenomeFloatViewComponent& AViewComp = Registry.get<FGenomeFloatViewComponent>(ParentA);
		const FGenomeFloatViewComponent& BViewComp = Registry.get<FGenomeFloatViewComponent>(ParentB);
		FGenomeFloatViewComponent& ChildViewComp = Registry.get<FGenomeFloatViewComponent>(ChildEntity);

		TArrayView<const float> AView = TArrayView<const float>(AViewComp.Values.GetData(), AViewComp.Values.Num());
		TArrayView<const float> BView = TArrayView<const float>(BViewComp.Values.GetData(), BViewComp.Values.Num());
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
	}

	// If any pairs remain unused, warn once
	if (PairIt != PairEnd)
	{
		int32 Remaining = 0;
		for (; PairIt != PairEnd; ++PairIt) { ++Remaining; }
		UE_LOG(LogTemp, Warning, TEXT("BreedFloatGenomesSystem: %d FBreedingPairComponent entities left unused."), Remaining);
	}
}
