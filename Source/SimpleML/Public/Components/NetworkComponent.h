// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "NeuralNetwork.h"
#include "NetworkComponent.generated.h"

// Move concrete network wrappers here so components can use them directly
USTRUCT(BlueprintType)
struct SIMPLEML_API FNeuralNetworkFloat
{
	GENERATED_BODY()

	TNeuralNetwork<float> Network;

	void Initialize(const TArray<FNeuralNetworkLayerDescriptor>& LayerDescriptors)
	{
		Network.Initialize(LayerDescriptors);
	}
};

USTRUCT(BlueprintType)
struct SIMPLEML_API FNeuralNetworkDouble
{
	GENERATED_BODY()

	TNeuralNetwork<double> Network;

	void Initialize(const TArray<FNeuralNetworkLayerDescriptor>& LayerDescriptors)
	{
		Network.Initialize(LayerDescriptors);
	}
};

