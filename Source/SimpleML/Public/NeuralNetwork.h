// NeuralNetwork.h
#pragma once

#include "CoreMinimal.h"
THIRD_PARTY_INCLUDES_START
#include "Dense"
THIRD_PARTY_INCLUDES_END
#include "NeuralNetwork.generated.h"

/**
 * Enum to specify the type of neural network layer
 */
UENUM(BlueprintType)
enum class ENeuronLayerType : uint8
{
    Feedforward UMETA(DisplayName = "Feedforward"),
    LSTM UMETA(DisplayName = "LSTM"),
    GRU UMETA(DisplayName = "GRU")
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

/**
 * Templated neural network structure with continuous memory layout for all layers.
 * Supports feedforward, LSTM, and GRU neuron types.
 * 
 * @tparam T The data type for weights and biases (float, double, etc.)
 */
template<typename T>
struct TNeuralNetwork
{
private:
    // Layer topology information
    TArray<FNeuralNetworkLayerDescriptor> LayerDescriptors;
    
    // Continuous memory storage for all weights and biases
    TArray<T> WeightsData;
    TArray<T> BiasesData;
    
    // Offset information for accessing layer data
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
    
    TArray<FLayerMemoryLayout> LayerLayouts;

public:
    TNeuralNetwork() = default;

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
        
        int32 TotalWeights = 0;
        int32 TotalBiases = 0;

        // Calculate memory requirements for each layer
        for (int32 i = 1; i < LayerDescriptors.Num(); ++i)
        {
            const int32 InputSize = LayerDescriptors[i - 1].NeuronCount;
            const int32 OutputSize = LayerDescriptors[i].NeuronCount;
            const ENeuronLayerType LayerType = LayerDescriptors[i].LayerType;

            FLayerMemoryLayout Layout;
            Layout.InputSize = InputSize;
            Layout.OutputSize = OutputSize;
            Layout.LayerType = LayerType;
            Layout.WeightsOffset = TotalWeights;
            Layout.BiasesOffset = TotalBiases;

            // Calculate weights and biases based on layer type
            switch (LayerType)
            {
            case ENeuronLayerType::Feedforward:
                Layout.WeightsCount = InputSize * OutputSize;
                Layout.BiasesCount = OutputSize;
                break;

            case ENeuronLayerType::LSTM:
                // LSTM has 4 gates (input, forget, cell, output)
                // Each gate needs weights for input and recurrent connections
                Layout.WeightsCount = 4 * OutputSize * (InputSize + OutputSize);
                Layout.BiasesCount = 4 * OutputSize;
                break;

            case ENeuronLayerType::GRU:
                // GRU has 3 gates (reset, update, new)
                Layout.WeightsCount = 3 * OutputSize * (InputSize + OutputSize);
                Layout.BiasesCount = 3 * OutputSize;
                break;

            default:
                Layout.WeightsCount = InputSize * OutputSize;
                Layout.BiasesCount = OutputSize;
                break;
            }

            TotalWeights += Layout.WeightsCount;
            TotalBiases += Layout.BiasesCount;

            LayerLayouts.Add(Layout);
        }

        // Allocate continuous memory for all weights and biases
        WeightsData.SetNumZeroed(TotalWeights);
        BiasesData.SetNumZeroed(TotalBiases);
        
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
                WeightsData[Index] = static_cast<T>(RandomStream.FRandRange(-1.0f, 1.0f) * StdDev);
            }

            // Initialize biases to small values
            for (int32 i = 0; i < Layout.BiasesCount; ++i)
            {
                const int32 Index = Layout.BiasesOffset + i;
                BiasesData[Index] = static_cast<T>(0.01);
            }
        }
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
        
        // For feedforward layers: rows = OutputSize, cols = InputSize
        if (Layout.LayerType == ENeuronLayerType::Feedforward)
        {
            return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
                &WeightsData[Layout.WeightsOffset],
                Layout.OutputSize,
                Layout.InputSize
            );
        }
        else
        {
            // For LSTM/GRU, return full weight matrix (caller needs to interpret gate structure)
            const int32 TotalInputSize = Layout.InputSize + Layout.OutputSize;
            const int32 GateCount = (Layout.LayerType == ENeuronLayerType::LSTM) ? 4 : 3;
            return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
                &WeightsData[Layout.WeightsOffset],
                GateCount * Layout.OutputSize,
                TotalInputSize
            );
        }
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
            &BiasesData[Layout.BiasesOffset],
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
        Eigen::Matrix<T, Eigen::Dynamic, 1> Activation = Input;

        for (int32 LayerIdx = 0; LayerIdx < LayerLayouts.Num(); ++LayerIdx)
        {
            const FLayerMemoryLayout& Layout = LayerLayouts[LayerIdx];

            if (Layout.LayerType == ENeuronLayerType::Feedforward)
            {
                auto Weights = GetWeightMatrix(LayerIdx);
                auto Biases = GetBiasVector(LayerIdx);
                
                // Linear transformation: y = Wx + b
                Activation = Weights * Activation + Biases;
                
                // Apply activation function (ReLU for hidden layers)
                if (LayerIdx < LayerLayouts.Num() - 1)
                {
                    Activation = Activation.array().max(0);  // ReLU
                }
            }
            else
            {
                // LSTM/GRU forward pass would go here
                // Left as placeholder for future implementation
                UE_LOG(LogTemp, Warning, TEXT("LSTM/GRU forward pass not yet implemented"));
            }
        }

        return Activation;
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
    int32 GetTotalWeightsCount() const { return WeightsData.Num(); }

    /**
     * Get total number of biases in the network
     */
    int32 GetTotalBiasesCount() const { return BiasesData.Num(); }

    /**
     * Get raw access to weights data (for serialization/deserialization)
     */
    TArray<T>& GetWeightsData() { return WeightsData; }
    const TArray<T>& GetWeightsData() const { return WeightsData; }

    /**
     * Get raw access to biases data (for serialization/deserialization)
     */
    TArray<T>& GetBiasesData() { return BiasesData; }
    const TArray<T>& GetBiasesData() const { return BiasesData; }
};

/**
 * USTRUCT wrapper for use in ECS and Blueprint systems
 * Note: USTRUCTs cannot be templated, so we provide specific type instantiations
 */
USTRUCT(BlueprintType)
struct FNeuralNetworkFloat
{
    GENERATED_BODY()

    TNeuralNetwork<float> Network;

    void Initialize(const TArray<FNeuralNetworkLayerDescriptor>& LayerDescriptors)
    {
        Network.Initialize(LayerDescriptors);
    }
};

USTRUCT(BlueprintType)
struct FNeuralNetworkDouble
{
    GENERATED_BODY()

    TNeuralNetwork<double> Network;

    void Initialize(const TArray<FNeuralNetworkLayerDescriptor>& LayerDescriptors)
    {
        Network.Initialize(LayerDescriptors);
    }
};