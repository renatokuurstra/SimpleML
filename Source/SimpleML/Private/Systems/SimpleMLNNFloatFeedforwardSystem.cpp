// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License.

#include "Systems/SimpleMLNNFloatFeedforwardSystem.h"

THIRD_PARTY_INCLUDES_START
#include "Dense"
THIRD_PARTY_INCLUDES_END

void USimpleMLNNFloatFeedforwardSystem::Update(float DeltaTime)
{
	// Operate on entities that have a network and the IO components
	auto View = GetView<FNeuralNetworkFloat, FNNInFLoatComp, FNNOutFloatComp>();
	for (auto Entity : View)
	{
		FNeuralNetworkFloat& NetComp = View.get<FNeuralNetworkFloat>(Entity);
		FNNInFLoatComp& In = View.get<FNNInFLoatComp>(Entity);
		FNNOutFloatComp& Out = View.get<FNNOutFloatComp>(Entity);

		const int32 InSize = NetComp.Network.GetInputSize();
		const int32 OutSize = NetComp.Network.GetOutputSize();

		// Validate inputs: must match network input size
		if (In.Values.Num() != InSize)
		{
			UE_LOG(LogTemp, Error, TEXT("FeedforwardSystem: input size mismatch. Expected %d, got %d."), InSize, In.Values.Num());
			// Keep outputs sized but zeroed if mismatch
			Out.Values.SetNumZeroed(OutSize);
			continue;
		}

		// Perform feedforward via the network API that accepts arrays
		if (!NetComp.Network.FeedforwardArray(In.Values, Out.Values))
		{
			// If it failed, ensure output is sized but zeroed to be safe
			Out.Values.SetNumZeroed(OutSize);
		}
	}
}
