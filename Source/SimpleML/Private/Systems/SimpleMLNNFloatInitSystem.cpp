// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License.

#include "Systems/SimpleMLNNFloatInitSystem.h"

void USimpleMLNNFloatInitSystem::Update_Implementation(float DeltaTime)
{
	// Iterate over all entities with a float neural network and initialize weights/biases only
	auto View = GetView<FNeuralNetworkFloat>();
	for (auto Entity : View)
	{
		FNeuralNetworkFloat& Comp = View.get<FNeuralNetworkFloat>(Entity);
		if (!Comp.Network.bIsInitialized)
		{
			// Initialize weights/biases with a unique seed per entity to ensure diversity in population.
			// Combine the global context seed with the entity index.
			const int32 EntityId = static_cast<int32>(entt::to_integral(Entity));
			const int32 FinalSeed = ContextSeed + EntityId;
			Comp.Network.InitializeWeightsUniform(static_cast<float>(-1.0f), static_cast<float>(1.0f), FinalSeed);
			Comp.Network.bIsInitialized = true;
		}
	}
}
