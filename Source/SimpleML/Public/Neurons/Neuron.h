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
    template<typename T>
    static void Feedforward(
        const Eigen::Ref<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>& Weights,
        const Eigen::Ref<const Eigen::Matrix<T, Eigen::Dynamic, 1>>& Biases,
        const Eigen::Ref<const Eigen::Matrix<T, Eigen::Dynamic, 1>>& Inputs,
        Eigen::Ref<Eigen::Matrix<T, Eigen::Dynamic, 1>> Outputs,
        bool bApplyHiddenActivation)
    {
        // y = W*x + b
        Outputs = Weights * Inputs + Biases;

        // Hidden layers use ReLU; output layer stays linear for now.
        if (bApplyHiddenActivation)
        {
            Outputs = Outputs.array().max(0);
        }
    }

    // Map helpers: construct Eigen views over network memory.
    template<typename T>
    static Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> GetWeightMatrix(
        const TArray<T>& WeightsData,
        const FLayerMemoryLayout& Layout)
    {
        return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
            const_cast<T*>(&WeightsData[Layout.WeightsOffset]),
            Layout.OutputSize,
            Layout.InputSize
        );
    }

    template<typename T>
    static Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>> GetBiasVector(
        const TArray<T>& BiasesData,
        const FLayerMemoryLayout& Layout)
    {
        return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>>(
            const_cast<T*>(&BiasesData[Layout.BiasesOffset]),
            Layout.BiasesCount
        );
    }

    // Full-network feedforward: the whole loop lives in the neuron type.
    template<typename T>
    static Eigen::Matrix<T, Eigen::Dynamic, 1> FeedforwardNetwork(
        const TArray<FLayerMemoryLayout>& LayerLayouts,
        const TArray<T>& WeightsData,
        const TArray<T>& BiasesData,
        const Eigen::Ref<const Eigen::Matrix<T, Eigen::Dynamic, 1>>& Input)
    {
        Eigen::Matrix<T, Eigen::Dynamic, 1> Activation = Input;
        for (int32 LayerIdx = 0; LayerIdx < LayerLayouts.Num(); ++LayerIdx)
        {
            const auto& Layout = LayerLayouts[LayerIdx];
            auto Weights = GetWeightMatrix<T>(WeightsData, Layout);
            auto Biases = GetBiasVector<T>(BiasesData, Layout);

            Eigen::Matrix<T, Eigen::Dynamic, 1> Next;
            Next.resize(Layout.OutputSize);
            const bool bHiddenLayer = (LayerIdx < LayerLayouts.Num() - 1);
            Feedforward<T>(Weights, Biases, Activation, Next, bHiddenLayer);
            Activation.swap(Next);
        }
        return Activation;
    }
};
