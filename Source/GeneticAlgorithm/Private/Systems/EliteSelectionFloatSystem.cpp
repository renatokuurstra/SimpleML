//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/EliteSelectionFloatSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"


bool UEliteSelectionFloatSystem::IsCandidate(entt::entity E, const FFitnessComponent& /*Fit*/) const
{
    return GetRegistry().all_of<FGenomeFloatViewComponent>(E);
}

void UEliteSelectionFloatSystem::CopyGenomeToElite(entt::entity Winner, entt::entity Elite, int32 /*FitnessIndex*/)
{
    auto& Registry = GetRegistry();

    const FGenomeFloatViewComponent& SrcView = Registry.get<FGenomeFloatViewComponent>(Winner);
    const int32 Len = SrcView.Values.Num();

    // Ensure owned storage on elite
    FEliteOwnedFloatGenome* Owned = nullptr;
    if (Registry.all_of<FEliteOwnedFloatGenome>(Elite))
    {
        Owned = &Registry.get<FEliteOwnedFloatGenome>(Elite);
    }
    else
    {
        Owned = &Registry.emplace<FEliteOwnedFloatGenome>(Elite);
    }
    Owned->Values.SetNum(Len, EAllowShrinking::No);
    if (Len > 0)
    {
        FMemory::Memcpy(Owned->Values.GetData(), SrcView.Values.GetData(), sizeof(float) * Len);
    }

    // Bind base view component to owned storage
    FGenomeFloatViewComponent* EliteView = nullptr;
    if (Registry.all_of<FGenomeFloatViewComponent>(Elite))
    {
        EliteView = &Registry.get<FGenomeFloatViewComponent>(Elite);
    }
    else
    {
        EliteView = &Registry.emplace<FGenomeFloatViewComponent>(Elite);
    }
    EliteView->Values = TArrayView<float>(Owned->Values.GetData(), Owned->Values.Num());
}
