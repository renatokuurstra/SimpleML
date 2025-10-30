#include "Systems/BreedingPairCleanupSystem.h"
#include "Components/BreedingPairComponent.h"

void UBreedingPairCleanupSystem::Update(float /*DeltaTime*/)
{
    auto& Registry = GetRegistry();

    // Destroy all entities that carry FBreedingPairComponent
    auto View = GetView<FBreedingPairComponent>();

    // Collect first to avoid iterator invalidation concerns
    TArray<entt::entity> ToDestroy;
    for (auto Entity : View)
    {
        Registry.destroy(Entity);
    }
}
