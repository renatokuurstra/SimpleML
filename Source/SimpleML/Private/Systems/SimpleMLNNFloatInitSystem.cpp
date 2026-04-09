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
			// Initialize weights/biases with a sensible default range. No IO responsibilities here.
			// Using 0 as seed for deterministic initialization in systems by default.
			Comp.Network.InitializeWeightsUniform(static_cast<float>(-1.0f), static_cast<float>(1.0f), 0);
			Comp.Network.bIsInitialized = true;
		}
	}
}
