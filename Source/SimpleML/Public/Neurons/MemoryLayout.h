//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"

// Forward declaration of enum (defined with UENUM in NeuralNetwork.h)
enum class ENeuronLayerType : uint8;

// Memory layout information for a single network layer
struct FLayerMemoryLayout
{
    int32 WeightsOffset;
    int32 WeightsCount;
    int32 BiasesOffset;
    int32 BiasesCount;
    int32 InputSize;
    int32 OutputSize;
    ENeuronLayerType LayerType;
};
