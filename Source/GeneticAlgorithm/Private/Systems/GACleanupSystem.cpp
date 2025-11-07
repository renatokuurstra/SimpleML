#include "Systems/GACleanupSystem.h"
#include "Components/BreedingPairComponent.h"
#include "Components/GenomeComponents.h"

void UGACleanupSystem::Update(float /*DeltaTime*/)
{
    auto& Registry = GetRegistry();

    // 1) Destroy all entities that carry FBreedingPairComponent (transient link entities)
    {
        auto PairView = GetView<FBreedingPairComponent>();
        for (auto Entity : PairView)
        {
            Registry.destroy(Entity);
        }
    }

    // 2) Remove FResetGenomeComponent from all entities so user can re-apply next generation
    {
        auto ResetView = GetView<FResetGenomeComponent>();
        for (auto Entity : ResetView)
        {
            // Safe to remove tag; entity continues to exist
            Registry.remove<FResetGenomeComponent>(Entity);
        }
    }
}
