// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License.

#include "Systems/SimpleMLNetworkInitSystem.h"

void USimpleMLNetworkInitSystem::Update(float DeltaTime)
{
	// Iterate over all FNetworkComponent instances
	auto View = GetView<FNetworkComponent>();
	for (auto Entity : View)
	{
		FNetworkComponent& Comp = View.get<FNetworkComponent>(Entity);
		if (!Comp.bIsInitialized)
		{
			// Ensure IO buffers match network sizes
			const int32 InSize = Comp.Network.Network.GetInputSize();
			const int32 OutSize = Comp.Network.Network.GetOutputSize();
			Comp.InputValues.SetNum(InSize);
			Comp.OutputValues.SetNum(OutSize);
			for (int32 i = 0; i < InSize; ++i) { Comp.InputValues[i] = 0.0f; }
			for (int32 i = 0; i < OutSize; ++i) { Comp.OutputValues[i] = 0.0f; }

			// Initialize weights/biases uniformly in [InitMin, InitMax]
			if (FMath::IsNearlyEqual(Comp.InitMin, Comp.InitMax))
			{
				Comp.Network.Network.FillWeightsBiases(Comp.InitMin);
			}
			else
			{
				Comp.Network.Network.InitializeWeightsUniform(Comp.InitMin, Comp.InitMax);
			}

			Comp.bIsInitialized = true;
		}
	}
}
