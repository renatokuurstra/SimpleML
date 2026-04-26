//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "VehicleTrainerConfig.h"
#include "VehicleLibrary.h"

TArray<FName> UVehicleTrainerConfig::GetResetReasonOptions() const
{
	return {
		UVehicleLibrary::ReasonTooFarFromSpline,
		UVehicleLibrary::ReasonNoProgress,
		UVehicleLibrary::ReasonTooSlow,
		UVehicleLibrary::ReasonBackwardStart
	};
}

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
	// Base inputs: 28
	//  1: Signed distance to spline
	//  1: Current orientation (Z cross product)
	//  N: Future orientations (FutureDotProductDistances.Num())
	//  1: Velocity magnitude
	// Physics / Kinematic inputs (13):
	//  1: Pitch angle
	//  1: Roll angle
	//  1: Wheels on ground ratio
	//  1: Height above/below spline
	//  1: Pitch angular velocity
	//  1: Yaw angular velocity
	//  1: Roll angular velocity
	//  1: Forward velocity component (along spline)
	//  1: Lateral velocity component (perpendicular to spline)
	//  1: Vertical velocity component
	//  1: Spline curvature ahead
	//  1: Spline curvature direction (signed)
	//  1: Spline progress (normalized 0-1)
	// Engine inputs (2):
	//  1: Normalized engine RPM
	//  1: Normalized current gear
	// Per-wheel inputs (4 wheels):
	//  4: Wheel contact state
	//  4: Wheel suspension compression
	// RecurrentInputCount: Recurrent feedback
	// NOTE: Steering/throttle/brake excluded — recurrent neurons already feed
	//       the NN's previous output back as input, so the network knows what
	//       it commanded last frame.
	// Total base: 1 + 1 + FutureDotProductDistances.Num() + 1 + 13 + 4 + 4 + 2 = 28
	return 1 + 1 + FutureDotProductDistances.Num() + 1 + 13 + 4 + 4 + 2 + RecurrentInputCount;
}

int32 UVehicleTrainerConfig::GetTotalOutputCount() const
{
	// VehicleOutputCount: Direct vehicle controls
	// RecurrentInputCount: We need the same number of outputs to feed back as inputs for recurrence
	return VehicleOutputCount + RecurrentInputCount;
}
