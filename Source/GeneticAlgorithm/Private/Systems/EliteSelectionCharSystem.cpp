//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/EliteSelectionCharSystem.h"

void UEliteSelectionCharSystem::Update(float DeltaTime)
{
    Super::Update(DeltaTime);

    ApplySelectionFor<FGenomeFloatViewComponent, FEliteSolutionCharComponent>();
}

void UEliteSelectionCharSystem::SaveEliteGenome(entt::entity SourceEntity, entt::entity EliteEntity)
{
    auto& Registry = GetRegistry();
    const FGenomeCharViewComponent& SrcComp = Registry.get<FGenomeCharViewComponent>(SourceEntity);
    FEliteSolutionCharComponent& DstComp = Registry.get<FEliteSolutionCharComponent>(EliteEntity);

    const TArrayView<char>& Src = SrcComp.Values;
    if (Src.Num() > 0)
    {
        DstComp.Values.SetNum(Src.Num(), /*bAllowShrinking*/ false);
        for (int32 i = 0; i < Src.Num(); ++i)
        {
            DstComp.Values[i] = static_cast<int8>(Src[i]);
        }
    }
    else
    {
        DstComp.Values.Reset();
    }
}
