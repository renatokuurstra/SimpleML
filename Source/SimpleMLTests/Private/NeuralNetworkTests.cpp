// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "NeuralNetwork.h"

TEST_CLASS(NeuralNetworkDeterminismTest, "SimpleML.NeuralNetwork.Determinism")
{
    TEST_METHOD(ShouldBeDeterministicWithSameSeed)
    {
        TArray<FNeuralNetworkLayerDescriptor> Layers;
        Layers.Add(FNeuralNetworkLayerDescriptor(3));
        Layers.Add(FNeuralNetworkLayerDescriptor(5));
        Layers.Add(FNeuralNetworkLayerDescriptor(2));

        TNeuralNetwork<float, FNeuron> Net1;
        Net1.Initialize(Layers, 42);

        TNeuralNetwork<float, FNeuron> Net2;
        Net2.Initialize(Layers, 42);

        const TArray<float>& Data1 = Net1.GetData();
        const TArray<float>& Data2 = Net2.GetData();

        ASSERT_THAT(AreEqual(Data1.Num(), Data2.Num(), TEXT("Both networks should have same data size")));
        
        for (int32 i = 0; i < Data1.Num(); ++i)
        {
            ASSERT_THAT(IsNear(Data1[i], Data2[i], 0.001f, FString::Printf(TEXT("Weights at index %d should be nearly equal with same seed"), i)));
        }
    }

    TEST_METHOD(ShouldBeDifferentWithDifferentSeeds)
    {
        TArray<FNeuralNetworkLayerDescriptor> Layers;
        Layers.Add(FNeuralNetworkLayerDescriptor(3));
        Layers.Add(FNeuralNetworkLayerDescriptor(5));
        Layers.Add(FNeuralNetworkLayerDescriptor(2));

        TNeuralNetwork<float, FNeuron> Net1;
        Net1.Initialize(Layers, 42);

        TNeuralNetwork<float, FNeuron> Net2;
        Net2.Initialize(Layers, 43);

        const TArray<float>& Data1 = Net1.GetData();
        const TArray<float>& Data2 = Net2.GetData();

        ASSERT_THAT(AreEqual(Data1.Num(), Data2.Num(), TEXT("Both networks should have same data size")));
        
        bool bAnyDifferent = false;
        for (int32 i = 0; i < Data1.Num(); ++i)
        {
            if (FMath::Abs(Data1[i] - Data2[i]) > 1e-6f)
            {
                bAnyDifferent = true;
                break;
            }
        }
        
        ASSERT_THAT(IsTrue(bAnyDifferent, TEXT("Weights should be different with different seeds")));
    }

    TEST_METHOD(ShouldUseZeroAsDefaultSeed)
    {
        TArray<FNeuralNetworkLayerDescriptor> Layers;
        Layers.Add(FNeuralNetworkLayerDescriptor(3));
        Layers.Add(FNeuralNetworkLayerDescriptor(2));

        TNeuralNetwork<float, FNeuron> Net1;
        Net1.Initialize(Layers, 0);

        TNeuralNetwork<float, FNeuron> Net2;
        Net2.Initialize(Layers); // Should use default 0

        const TArray<float>& Data1 = Net1.GetData();
        const TArray<float>& Data2 = Net2.GetData();

        for (int32 i = 0; i < Data1.Num(); ++i)
        {
            ASSERT_THAT(IsNear(Data1[i], Data2[i], 0.001f, FString::Printf(TEXT("Weights at index %d should match default seed 0"), i)));
        }
    }
};
