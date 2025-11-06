//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "Neurons/MemoryLayout.h"

THIRD_PARTY_INCLUDES_START
#include "Dense"
THIRD_PARTY_INCLUDES_END

// Plain neuron type: no USTRUCT as it's not needed for reflection or serialization
struct SIMPLEML_API FNeuron
{
    // Basic feedforward neuron operation working directly on Eigen data.
    // Activation: hyperbolic tangent (tanh) applied to ALL layers (hidden and output).
    template<typename T>
    static void Feedforward(
        const Eigen::Ref<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>& Weights,
        const Eigen::Ref<const Eigen::Matrix<T, Eigen::Dynamic, 1>>& Biases,
        const Eigen::Ref<const Eigen::Matrix<T, Eigen::Dynamic, 1>>& Inputs,
        Eigen::Ref<Eigen::Matrix<T, Eigen::Dynamic, 1>> Outputs)
    {
        // y = tanh(W*x + b)
        Outputs = (Weights * Inputs + Biases).array().tanh().matrix();
    }

    // Map helpers: construct Eigen views over network memory.
    template<typename T>
    static Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> GetWeightMatrix(
        const TArray<T>& Data,
        const FLayerMemoryLayout& Layout)
    {
        return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
            const_cast<T*>(&Data[Layout.WeightsOffset]),
            Layout.OutputSize,
            Layout.InputSize
        );
    }

    template<typename T>
    static Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>> GetBiasVector(
        const TArray<T>& Data,
        const FLayerMemoryLayout& Layout)
    {
        return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>>(
            const_cast<T*>(&Data[Layout.BiasesOffset]),
            Layout.BiasesCount
        );
    }

    // Full-network feedforward: the whole loop lives in the neuron type.
    template<typename T>
    static Eigen::Matrix<T, Eigen::Dynamic, 1> FeedforwardNetwork(
        const TArray<FLayerMemoryLayout>& LayerLayouts,
        const TArray<T>& Data,
        const Eigen::Ref<const Eigen::Matrix<T, Eigen::Dynamic, 1>>& Input)
    {
        Eigen::Matrix<T, Eigen::Dynamic, 1> Activation = Input;
        for (int32 LayerIdx = 0; LayerIdx < LayerLayouts.Num(); ++LayerIdx)
        {
            const auto& Layout = LayerLayouts[LayerIdx];
            auto Weights = GetWeightMatrix<T>(Data, Layout);
            auto Biases = GetBiasVector<T>(Data, Layout);

            Eigen::Matrix<T, Eigen::Dynamic, 1> Next;
            Next.resize(Layout.OutputSize);
            Feedforward<T>(Weights, Biases, Activation, Next);
            Activation.swap(Next);
        }
        return Activation;
    }
};
