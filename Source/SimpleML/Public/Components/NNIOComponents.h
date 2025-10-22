// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "NNIOComponents.generated.h"

// Minimal input component carrying a float array for NN inputs
USTRUCT(BlueprintType)
struct SIMPLEML_API FNNInFLoatComp
{
	GENERATED_BODY()

	// Raw input values for the neural network
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleML|NN")
	TArray<float> Values;
};

// Minimal output component carrying a float array for NN outputs
USTRUCT(BlueprintType)
struct SIMPLEML_API FNNOutFloatComp
{
	GENERATED_BODY()

	// Raw output values from the neural network
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleML|NN")
	TArray<float> Values;
};
