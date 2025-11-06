//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
THIRD_PARTY_INCLUDES_START
#include "Dense"
THIRD_PARTY_INCLUDES_END
#include "Neurons/MemoryLayout.h"
#include "Neurons/Neuron.h"
#include "NeuralNetwork.generated.h"


/**
 * Enum to specify the type of neural network layer
 */
UENUM(BlueprintType)
enum class ENeuronLayerType : uint8
{
    Feedforward UMETA(DisplayName = "Feedforward")
};

/**
 * Structure defining a single layer in the neural network
 */
USTRUCT(BlueprintType)
struct FNeuralNetworkLayerDescriptor
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network")
    int32 NeuronCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network")
    ENeuronLayerType LayerType = ENeuronLayerType::Feedforward;

    FNeuralNetworkLayerDescriptor() = default;
    
    FNeuralNetworkLayerDescriptor(int32 InNeuronCount, ENeuronLayerType InLayerType = ENeuronLayerType::Feedforward)
        : NeuronCount(InNeuronCount), LayerType(InLayerType)
    {
    }
};


// Neuron implementation moved to Neurons/Neuron.h

/**
 * Templated neural network structure with continuous memory layout for all layers.
 * Supports feedforward, LSTM, and GRU neuron types.
 * 
 * @tparam T The data type for weights and biases (float, double, etc.)
 */
template<typename T, typename TNeuron>
struct TNeuralNetwork
{
private:
    // Layer topology information
    TArray<FNeuralNetworkLayerDescriptor> LayerDescriptors;
    
    // Single contiguous memory storage for all weights and biases (layer offsets point into this buffer)
    TArray<T> Data;
    
    // Offset information for accessing layer data
private:
    TArray<FLayerMemoryLayout> LayerLayouts;


public:
    TNeuralNetwork() = default;
    bool bIsInitialized = false;
    
    /**
     * Initialize the neural network with given layer descriptors
     * @param InLayerDescriptors Array of layer descriptors defining the network architecture
     */
    void Initialize(const TArray<FNeuralNetworkLayerDescriptor>& InLayerDescriptors)
    {
        if (InLayerDescriptors.Num() < 2)
        {
            UE_LOG(LogTemp, Error, TEXT("Neural network must have at least 2 layers (input and output)"));
            return;
        }

        LayerDescriptors = InLayerDescriptors;
        LayerLayouts.Reset();
        
        int32 TotalData = 0;

        // Calculate memory requirements for each layer and assign offsets within a single contiguous buffer
        for (int32 i = 1; i < LayerDescriptors.Num(); ++i)
        {
            const int32 InputSize = LayerDescriptors[i - 1].NeuronCount;
            const int32 OutputSize = LayerDescriptors[i].NeuronCount;
            const ENeuronLayerType LayerType = LayerDescriptors[i].LayerType;

            FLayerMemoryLayout Layout;
            Layout.InputSize = InputSize;
            Layout.OutputSize = OutputSize;
            Layout.LayerType = LayerType;

            // Weights for this layer start at current TotalData
            Layout.WeightsOffset = TotalData;
            Layout.WeightsCount = InputSize * OutputSize;
            TotalData += Layout.WeightsCount;

            // Biases immediately follow weights in the single buffer
            Layout.BiasesOffset = TotalData;
            Layout.BiasesCount = OutputSize;
            TotalData += Layout.BiasesCount;

            LayerLayouts.Add(Layout);
        }

        // Allocate single contiguous memory for all weights and biases
        Data.SetNumZeroed(TotalData);
        
        InitializeWeights();
    }

    /**
     * Initialize weights using Xavier/He initialization
     */
    void InitializeWeights()
    {
        FRandomStream RandomStream(FDateTime::Now().GetTicks());

        for (int32 LayerIdx = 0; LayerIdx < LayerLayouts.Num(); ++LayerIdx)
        {
            const FLayerMemoryLayout& Layout = LayerLayouts[LayerIdx];
            
            // Xavier initialization: std = sqrt(2.0 / (InputSize + OutputSize))
            const T StdDev = FMath::Sqrt(2.0 / (Layout.InputSize + Layout.OutputSize));

            // Initialize weights
            for (int32 i = 0; i < Layout.WeightsCount; ++i)
            {
                const int32 Index = Layout.WeightsOffset + i;
                Data[Index] = static_cast<T>(RandomStream.FRandRange(-1.0f, 1.0f) * StdDev);
            }

            // Initialize biases to small values
            for (int32 i = 0; i < Layout.BiasesCount; ++i)
            {
                const int32 Index = Layout.BiasesOffset + i;
                Data[Index] = static_cast<T>(0.01);
            }
        }
    }

    // Fill weights and biases with a uniform random in [Min, Max]
    void InitializeWeightsUniform(T Min, T Max)
    {
        FRandomStream RandomStream(FDateTime::Now().GetTicks());
        for (int32 LayerIdx = 0; LayerIdx < LayerLayouts.Num(); ++LayerIdx)
        {
            const FLayerMemoryLayout& Layout = LayerLayouts[LayerIdx];
            for (int32 i = 0; i < Layout.WeightsCount; ++i)
            {
                const int32 Index = Layout.WeightsOffset + i;
                Data[Index] = static_cast<T>(RandomStream.FRandRange((float)Min, (float)Max));
            }
            for (int32 i = 0; i < Layout.BiasesCount; ++i)
            {
                const int32 Index = Layout.BiasesOffset + i;
                Data[Index] = static_cast<T>(RandomStream.FRandRange((float)Min, (float)Max));
            }
        }
    }

    // Deterministic fill (Min==Max) convenience
    void FillWeightsBiases(T Value)
    {
        for (int32 LayerIdx = 0; LayerIdx < LayerLayouts.Num(); ++LayerIdx)
        {
            const FLayerMemoryLayout& Layout = LayerLayouts[LayerIdx];
            for (int32 i = 0; i < Layout.WeightsCount; ++i)
            {
                Data[Layout.WeightsOffset + i] = Value;
            }
            for (int32 i = 0; i < Layout.BiasesCount; ++i)
            {
                Data[Layout.BiasesOffset + i] = Value;
            }
        }
    }

    int32 GetInputSize() const
    {
        return LayerDescriptors.Num() > 0 ? LayerDescriptors[0].NeuronCount : 0;
    }
    

    int32 GetOutputSize() const
    {
        return LayerDescriptors.Num() > 0 ? LayerDescriptors.Last().NeuronCount : 0;
    }

    // Perform a feedforward pass taking inputs by array ref and writing outputs into OutOutputs.
    // Returns true on success (when input count matches network input size), false otherwise.
    bool FeedforwardArray(const TArray<T>& InInputs, TArray<T>& OutOutputs)
    {
        const int32 ExpectedIn = GetInputSize();
        const int32 OutSize = GetOutputSize();
        if (ExpectedIn <= 0 || OutSize <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("FeedforwardArray called on an uninitialized network (no layers)."));
            OutOutputs.Reset();
            return false;
        }
        if (InInputs.Num() != ExpectedIn)
        {
            UE_LOG(LogTemp, Warning, TEXT("FeedforwardArray input size mismatch. Expected %d, got %d."), ExpectedIn, InInputs.Num());
            return false;
        }

        // Map input array to Eigen vector
        Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, 1>> InputVec(InInputs.GetData(), InInputs.Num());
        Eigen::Matrix<T, Eigen::Dynamic, 1> OutVec = Forward(InputVec);

        OutOutputs.SetNumUninitialized(OutVec.size());
        for (int32 i = 0; i < OutVec.size(); ++i)
        {
            OutOutputs[i] = OutVec(i);
        }
        return true;
    }
    
    /**
     * Get weight matrix for a specific layer as Eigen matrix map
     * @param LayerIndex Index of the layer (0-based, after input layer)
     * @return Eigen matrix map view of the weights
     */
    Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> GetWeightMatrix(int32 LayerIndex)
    {
        check(LayerIndex >= 0 && LayerIndex < LayerLayouts.Num());
        const FLayerMemoryLayout& Layout = LayerLayouts[LayerIndex];
        return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
            &Data[Layout.WeightsOffset],
            Layout.OutputSize,
            Layout.InputSize
        );
    }

    /**
     * Get bias vector for a specific layer as Eigen vector map
     * @param LayerIndex Index of the layer (0-based, after input layer)
     * @return Eigen vector map view of the biases
     */
    Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>> GetBiasVector(int32 LayerIndex)
    {
        check(LayerIndex >= 0 && LayerIndex < LayerLayouts.Num());
        const FLayerMemoryLayout& Layout = LayerLayouts[LayerIndex];
        
        return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>>(
            &Data[Layout.BiasesOffset],
            Layout.BiasesCount
        );
    }

    /**
     * Forward pass through the network (basic feedforward implementation)
     * @param Input Input vector
     * @return Output vector
     */
    Eigen::Matrix<T, Eigen::Dynamic, 1> Forward(const Eigen::Matrix<T, Eigen::Dynamic, 1>& Input)
    {
        return TNeuron::template FeedforwardNetwork<T>(LayerLayouts, Data, Input);
    }

    /**
     * Get the number of layers (excluding input layer)
     */
    int32 GetNumLayers() const { return LayerLayouts.Num(); }

    /**
     * Get layer descriptors
     */
    const TArray<FNeuralNetworkLayerDescriptor>& GetLayerDescriptors() const { return LayerDescriptors; }

    /**
     * Get total number of weights in the network
     */
    int32 GetTotalWeightsCount() const {
        int32 Sum = 0;
        for (const FLayerMemoryLayout& L : LayerLayouts) { Sum += L.WeightsCount; }
        return Sum;
    }

    /**
     * Get total number of biases in the network
     */
    int32 GetTotalBiasesCount() const {
        int32 Sum = 0;
        for (const FLayerMemoryLayout& L : LayerLayouts) { Sum += L.BiasesCount; }
        return Sum;
    }

    /**
     * Unified contiguous parameter buffer access
     */
    TArray<T>& GetData() { return Data; }
    const TArray<T>& GetData() const { return Data; }

    /**
     * Lightweight array views over the whole parameter buffer
     */
    TArrayView<T> GetDataView() { return TArrayView<T>(Data.GetData(), Data.Num()); }
    TArrayView<const T> GetDataView() const { return TArrayView<const T>(Data.GetData(), Data.Num()); }
};
