//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/BreedCharGenomesSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/BreedingPairComponent.h"
#include "Math/UnrealMathUtility.h"

void UBreedCharGenomesSystem::Update(float /*DeltaTime*/)
{
    auto& Registry = GetRegistry();

    // Iterator-based views (no caching), mirroring UBreedFloatGenomesSystem
    auto ResetView = GetView<FResetGenomeComponent, FGenomeCharViewComponent>();
    auto PairView = GetView<FBreedingPairComponent>();

    if (ResetView.begin() == ResetView.end())
    {
        return; // nothing to reset
    }
    if (PairView.begin() == PairView.end())
    {
        UE_LOG(LogTemp, Warning, TEXT("BreedCharGenomesSystem: no FBreedingPairComponent entities available."));
        return;
    }

    // RNG setup
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
            UE_LOG(LogTemp, Warning, TEXT("BreedCharGenomesSystem: ran out of FBreedingPairComponent entities after processing %d reset entities."), Index);
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
            UE_LOG(LogTemp, Warning, TEXT("BreedCharGenomesSystem: invalid parent(s) in pair at reset index %d"), Index);
            continue;
        }

        // Validate required components
        if (!Registry.all_of<FGenomeCharViewComponent>(ParentA) || !Registry.all_of<FGenomeCharViewComponent>(ParentB) || !Registry.all_of<FGenomeCharViewComponent>(ChildEntity))
        {
            UE_LOG(LogTemp, Warning, TEXT("BreedCharGenomesSystem: missing FGenomeCharViewComponent on parent or child (reset index=%d)"), Index);
            continue;
        }

        const FGenomeCharViewComponent& AViewComp = Registry.get<FGenomeCharViewComponent>(ParentA);
        const FGenomeCharViewComponent& BViewComp = Registry.get<FGenomeCharViewComponent>(ParentB);
        FGenomeCharViewComponent& ChildViewComp = Registry.get<FGenomeCharViewComponent>(ChildEntity);

        TArrayView<const char> AView = TArrayView<const char>(AViewComp.Values.GetData(), AViewComp.Values.Num());
        TArrayView<const char> BView = TArrayView<const char>(BViewComp.Values.GetData(), BViewComp.Values.Num());
        TArrayView<char> CView = ChildViewComp.Values;

        const int32 GeneCount = FMath::Min3(AView.Num(), BView.Num(), CView.Num());
        if (GeneCount <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("BreedCharGenomesSystem: zero-length genome at reset index=%d"), Index);
            continue;
        }

        int32 i = 0;
        // Process in batches of 32 genes using a 32-bit random mask to decide per-gene parent
        for (; i + 32 <= GeneCount; i += 32)
        {
            uint32 Mask;
            if (RngPtr)
            {
                // Compose a 32-bit mask from two 16-bit draws deterministically
                const uint32 Hi = static_cast<uint32>(RngPtr->RandRange(0, 65535));
                const uint32 Lo = static_cast<uint32>(RngPtr->RandRange(0, 65535));
                Mask = (Hi << 16) | Lo;
            }
            else
            {
                // Use engine RNG: combine two calls to produce 32 bits
                const uint32 Hi = static_cast<uint32>(FMath::Rand()) & 0xFFFFu;
                const uint32 Lo = static_cast<uint32>(FMath::Rand()) & 0xFFFFu;
                Mask = (Hi << 16) | Lo;
            }

            // Unroll a bit for throughput; choose per bit
            CView[i + 0]  = ((Mask >> 0)  & 1u) ? AView[i + 0]  : BView[i + 0];
            CView[i + 1]  = ((Mask >> 1)  & 1u) ? AView[i + 1]  : BView[i + 1];
            CView[i + 2]  = ((Mask >> 2)  & 1u) ? AView[i + 2]  : BView[i + 2];
            CView[i + 3]  = ((Mask >> 3)  & 1u) ? AView[i + 3]  : BView[i + 3];
            CView[i + 4]  = ((Mask >> 4)  & 1u) ? AView[i + 4]  : BView[i + 4];
            CView[i + 5]  = ((Mask >> 5)  & 1u) ? AView[i + 5]  : BView[i + 5];
            CView[i + 6]  = ((Mask >> 6)  & 1u) ? AView[i + 6]  : BView[i + 6];
            CView[i + 7]  = ((Mask >> 7)  & 1u) ? AView[i + 7]  : BView[i + 7];
            CView[i + 8]  = ((Mask >> 8)  & 1u) ? AView[i + 8]  : BView[i + 8];
            CView[i + 9]  = ((Mask >> 9)  & 1u) ? AView[i + 9]  : BView[i + 9];
            CView[i + 10] = ((Mask >> 10) & 1u) ? AView[i + 10] : BView[i + 10];
            CView[i + 11] = ((Mask >> 11) & 1u) ? AView[i + 11] : BView[i + 11];
            CView[i + 12] = ((Mask >> 12) & 1u) ? AView[i + 12] : BView[i + 12];
            CView[i + 13] = ((Mask >> 13) & 1u) ? AView[i + 13] : BView[i + 13];
            CView[i + 14] = ((Mask >> 14) & 1u) ? AView[i + 14] : BView[i + 14];
            CView[i + 15] = ((Mask >> 15) & 1u) ? AView[i + 15] : BView[i + 15];
            CView[i + 16] = ((Mask >> 16) & 1u) ? AView[i + 16] : BView[i + 16];
            CView[i + 17] = ((Mask >> 17) & 1u) ? AView[i + 17] : BView[i + 17];
            CView[i + 18] = ((Mask >> 18) & 1u) ? AView[i + 18] : BView[i + 18];
            CView[i + 19] = ((Mask >> 19) & 1u) ? AView[i + 19] : BView[i + 19];
            CView[i + 20] = ((Mask >> 20) & 1u) ? AView[i + 20] : BView[i + 20];
            CView[i + 21] = ((Mask >> 21) & 1u) ? AView[i + 21] : BView[i + 21];
            CView[i + 22] = ((Mask >> 22) & 1u) ? AView[i + 22] : BView[i + 22];
            CView[i + 23] = ((Mask >> 23) & 1u) ? AView[i + 23] : BView[i + 23];
            CView[i + 24] = ((Mask >> 24) & 1u) ? AView[i + 24] : BView[i + 24];
            CView[i + 25] = ((Mask >> 25) & 1u) ? AView[i + 25] : BView[i + 25];
            CView[i + 26] = ((Mask >> 26) & 1u) ? AView[i + 26] : BView[i + 26];
            CView[i + 27] = ((Mask >> 27) & 1u) ? AView[i + 27] : BView[i + 27];
            CView[i + 28] = ((Mask >> 28) & 1u) ? AView[i + 28] : BView[i + 28];
            CView[i + 29] = ((Mask >> 29) & 1u) ? AView[i + 29] : BView[i + 29];
            CView[i + 30] = ((Mask >> 30) & 1u) ? AView[i + 30] : BView[i + 30];
            CView[i + 31] = ((Mask >> 31) & 1u) ? AView[i + 31] : BView[i + 31];
        }

        // Tail processing
        for (; i < GeneCount; ++i)
        {
            const bool bPickA = (RngPtr ? (RngPtr->RandRange(0, 1) == 0) : (FMath::RandRange(0, 1) == 0));
            CView[i] = bPickA ? AView[i] : BView[i];
        }
    }

    // Warn if any pairs remain unused
    if (PairIt != PairEnd)
    {
        int32 Remaining = 0;
        for (; PairIt != PairEnd; ++PairIt) { ++Remaining; }
        UE_LOG(LogTemp, Warning, TEXT("BreedCharGenomesSystem: %d FBreedingPairComponent entities left unused."), Remaining);
    }
}
