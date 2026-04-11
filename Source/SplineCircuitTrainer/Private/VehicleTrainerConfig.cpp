//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "VehicleTrainerConfig.h"

TArray<FNeuralNetworkLayerDescriptor> UVehicleTrainerConfig::GetNNLayerDescriptors() const
{
	TArray<FNeuralNetworkLayerDescriptor> Descriptors;

	// Input Layer
	Descriptors.Add(FNeuralNetworkLayerDescriptor(GetTotalInputCount(), ENeuronLayerType::Feedforward));

	// Hidden Layers
	for (int32 HiddenSize : HiddenLayerSizes)
	{
		Descriptors.Add(FNeuralNetworkLayerDescriptor(HiddenSize, ENeuronLayerType::Feedforward));
	}

	// Output Layer
	Descriptors.Add(FNeuralNetworkLayerDescriptor(GetTotalOutputCount(), ENeuronLayerType::Feedforward));

	return Descriptors;
}

int32 UVehicleTrainerConfig::GetTotalInputCount() const
{
	// 1: Distance to spline
	// 1: Current dot product
	// FutureDotProductDistances.Num(): Future dot products
	// 1: Velocity
	// RecurrentInputCount: Recurrence
	return 1 + 1 + FutureDotProductDistances.Num() + 1 + RecurrentInputCount;
}

int32 UVehicleTrainerConfig::GetTotalOutputCount() const
{
	// VehicleOutputCount: Direct vehicle controls
	// RecurrentInputCount: We need the same number of outputs to feed back as inputs for recurrence
	return VehicleOutputCount + RecurrentInputCount;
}
