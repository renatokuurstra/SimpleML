//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/EliteSelectionCharSystem.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"


bool UEliteSelectionCharSystem::IsCandidate(entt::entity E, const FFitnessComponent& /*Fit*/) const
{
    // Candidate must have a char genome view
    return GetRegistry().all_of<FGenomeCharViewComponent>(E);
}

void UEliteSelectionCharSystem::CopyGenomeToElite(entt::entity Winner, entt::entity Elite, int32 /*FitnessIndex*/)
{
    auto& Registry = GetRegistry();

    const FGenomeCharViewComponent& SrcView = Registry.get<FGenomeCharViewComponent>(Winner);
    const int32 Len = SrcView.Values.Num();

    // Ensure owned storage on elite
    FEliteOwnedCharGenome* Owned = nullptr;
    if (Registry.all_of<FEliteOwnedCharGenome>(Elite))
    {
        Owned = &Registry.get<FEliteOwnedCharGenome>(Elite);
    }
    else
    {
        Owned = &Registry.emplace<FEliteOwnedCharGenome>(Elite);
    }
    Owned->Values.SetNum(Len, EAllowShrinking::No);
    if (Len > 0)
    {
        FMemory::Memcpy(Owned->Values.GetData(), reinterpret_cast<const void*>(SrcView.Values.GetData()), sizeof(int8) * Len);
    }

    // Bind base view component to owned storage
    FGenomeCharViewComponent* EliteView = nullptr;
    if (Registry.all_of<FGenomeCharViewComponent>(Elite))
    {
        EliteView = &Registry.get<FGenomeCharViewComponent>(Elite);
    }
    else
    {
        EliteView = &Registry.emplace<FGenomeCharViewComponent>(Elite);
    }
    EliteView->Values = TArrayView<char>(reinterpret_cast<char*>(Owned->Values.GetData()), Owned->Values.Num());
}
