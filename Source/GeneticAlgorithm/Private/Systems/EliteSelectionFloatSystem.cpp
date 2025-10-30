//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/EliteSelectionFloatSystem.h"

void UEliteSelectionFloatSystem::Update(float DeltaTime)
{
    Super::Update(DeltaTime);
    
    ApplySelectionFor<FGenomeFloatViewComponent, FEliteSolutionFloatComponent>();
}

void UEliteSelectionFloatSystem::SaveEliteGenome(entt::entity SourceEntity, entt::entity EliteEntity)
{
    auto& Registry = GetRegistry();
    const FGenomeFloatViewComponent& SrcComp = Registry.get<FGenomeFloatViewComponent>(SourceEntity);
    FEliteSolutionFloatComponent& DstComp = Registry.get<FEliteSolutionFloatComponent>(EliteEntity);

    const TArrayView<float>& Src = SrcComp.Values;
    if (Src.Num() > 0)
    {
        DstComp.Values.SetNum(Src.Num(), /*bAllowShrinking*/ false);
        for (int32 i = 0; i < Src.Num(); ++i)
        {
            DstComp.Values[i] = Src[i];
        }
    }
    else
    {
        DstComp.Values.Reset();
    }
}
