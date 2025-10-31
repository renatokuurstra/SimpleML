// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/MutationCharGenomeSystem.h"

#include "Components/GenomeComponents.h"
#include "Math/UnrealMathUtility.h"

void UMutationCharGenomeSystem::Update(float /*DeltaTime*/)
{
    auto& Registry = GetRegistry();

    // Iterate directly over entities with a char genome view
    auto View = GetView<FGenomeCharViewComponent>();
    if (View.begin() == View.end())
    {
        return;
    }

    // RNG: deterministic if RandomSeed != 0
    FRandomStream Rng;
    FRandomStream* RngPtr = nullptr;
    if (RandomSeed != 0)
    {
        Rng.Initialize(RandomSeed);
        RngPtr = &Rng;
    }

    // Sanitize probability
    const float P = FMath::Clamp(BitFlipProbability, 0.0f, 1.0f);
    if (P <= 0.0f)
    {
        return; // nothing to do
    }

    // Precompute ln(1 - p) for geometric skipping; handle p = 1 separately
    const bool bFlipAll = (P >= 1.0f - KINDA_SMALL_NUMBER);
    const float Log1mP = bFlipAll ? 0.0f : FMath::Loge(1.0f - P);

    for (auto It = View.begin(), End = View.end(); It != End; ++It)
    {
        const entt::entity Entity = *It;
        FGenomeCharViewComponent& ViewComp = Registry.get<FGenomeCharViewComponent>(Entity);
        TArrayView<char>& ValuesChar = ViewComp.Values;
        const int32 NumBytes = ValuesChar.Num();
        if (NumBytes <= 0)
        {
            continue;
        }

        uint8* const Bytes = reinterpret_cast<uint8*>(ValuesChar.GetData());
        const int64 TotalBits = static_cast<int64>(NumBytes) * 8;

        if (bFlipAll)
        {
            // Flip every bit (XOR with 0xFF per byte)
            for (int32 i = 0; i < NumBytes; ++i)
            {
                Bytes[i] ^= 0xFFu;
            }
            continue;
        }

        // Geometric skipping over bit indices
        int64 BitIndex = -1; // Start before 0 so first increment lands on first candidate
        while (true)
        {
            // Sample U in (0,1]; avoid 0 to keep log defined. Use a tiny lower bound.
            const float U = RngPtr ? FMath::Max(1e-12f, RngPtr->FRand()) : FMath::Max(1e-12f, FMath::FRand());
            const double Skip = FMath::FloorToDouble(FMath::Loge(U) / Log1mP);
            BitIndex += static_cast<int64>(Skip) + 1;
            if (BitIndex >= TotalBits)
            {
                break;
            }

            const int64 ByteIndex = BitIndex >> 3;      // divide by 8
            const int32 BitInByte = static_cast<int32>(BitIndex & 7); // mod 8
            const uint8 Mask = static_cast<uint8>(1u << BitInByte);
            Bytes[ByteIndex] ^= Mask;
        }
    }
}
