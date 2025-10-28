#include "Systems/EliteSelectionSystem.h"

void UEliteSelectionSystem::Update(float /*DeltaTime*/)
{
    // Basic placeholder: compute indices of top-N elites (no side effects yet)
    auto FitnessArray = GetComponentArray<FFitnessComponent>();
    if (FitnessArray.Num() == 0)
    {
        return;
    }

    const TArray<float>& Fitness = FitnessArray[0].Fitness;
    const int32 Count = Fitness.Num();
    if (Count == 0)
    {
        return;
    }

    const int32 N = FMath::Clamp(EliteCount, 1, Count);

    // Make an index array 0..Count-1 and partially select top N
    TArray<int32> Indices;
    Indices.Reserve(Count);
    for (int32 i = 0; i < Count; ++i) { Indices.Add(i); }

    Indices.Sort([this, &Fitness](int32 A, int32 B)
    {
        const bool bABetter = bHigherIsBetter ? (Fitness[A] > Fitness[B]) : (Fitness[A] < Fitness[B]);
        if (bABetter) return true;
        if (Fitness[A] == Fitness[B]) return A < B;
        return false;
    });

    // Truncate to N (placeholder; future: write to a selection buffer component)
    if (Indices.Num() > N)
    {
        Indices.SetNum(N, /*bAllowShrinking*/ false);
    }
}
