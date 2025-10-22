// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "NeuralNetwork.h"
#include "NetworkComponent.generated.h"

/**
 * ECS component storing a simple neural network instance and IO buffers.
 * - Defaults to zeroed inputs/outputs.
 * - bIsInitialized controls whether init system should run weight/bias initialization.
 */
USTRUCT(BlueprintType)
struct SIMPLEML_API FNetworkComponent
{
	GENERATED_BODY()

public:
	// Network instance (float precision by default)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SimpleML|Network")
	FNeuralNetworkFloat Network;

	// Whether the initialization system already initialized weights/biases.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SimpleML|Network")
	bool bIsInitialized = false;

	// Range used by the initialization system (Uniform[InitMin, InitMax])
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SimpleML|Network")
	float InitMin = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SimpleML|Network")
	float InitMax = +1.0f;

	// Input and Output neuron values (SoA style, plain arrays)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SimpleML|Network")
	TArray<float> InputValues;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SimpleML|Network")
	TArray<float> OutputValues;

	// API to set/get IO values
	void SetInputs(const TArray<float>& InValues)
	{
		InputValues = InValues;
	}

	const TArray<float>& GetOutputs() const
	{
		return OutputValues;
	}
};
