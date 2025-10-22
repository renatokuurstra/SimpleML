// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License.

#include "Systems/SimpleMLFeedforwardSystem.h"

THIRD_PARTY_INCLUDES_START
#include "Dense"
THIRD_PARTY_INCLUDES_END

void USimpleMLFeedforwardSystem::Update(float DeltaTime)
{
	auto View = GetView<FNetworkComponent>();
	for (auto Entity : View)
	{
		FNetworkComponent& Comp = View.get<FNetworkComponent>(Entity);
		const int32 InSize = Comp.Network.Network.GetInputSize();
		const int32 OutSize = Comp.Network.Network.GetOutputSize();
		if (Comp.InputValues.Num() != InSize)
		{
			// Resize and pad with zeros if needed
			Comp.InputValues.SetNum(InSize);
			for (int32 i = 0; i < InSize; ++i) { if (!Comp.InputValues.IsValidIndex(i)) Comp.InputValues[i] = 0.0f; }
		}
		Comp.OutputValues.SetNum(OutSize);

		// Map inputs to Eigen vector
		Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, 1>> InputVec(Comp.InputValues.GetData(), InSize);
		Eigen::Matrix<float, Eigen::Dynamic, 1> Result = Comp.Network.Network.Forward(InputVec);

		// Copy back to OutputValues
		for (int32 i = 0; i < OutSize && i < Result.size(); ++i)
		{
			Comp.OutputValues[i] = Result[i];
		}
	}
}
