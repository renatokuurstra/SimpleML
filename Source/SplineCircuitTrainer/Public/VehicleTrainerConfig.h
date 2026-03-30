//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NeuralNetwork.h"
#include "VehicleTrainerConfig.generated.h"

class APawn;

/**
 * UVehicleTrainerConfig
 * Configuration data for the vehicle trainer.
 */
UCLASS(BlueprintType)
class SPLINECIRCUITTRAINER_API UVehicleTrainerConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	TSubclassOf<APawn> VehiclePawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	int32 Population = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network")
	TArray<FNeuralNetworkLayerDescriptor> NNLayerDescriptors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network", meta = (Units = "ms"))
	float NetworkUpdateFrequencyMS = 20.0f;
};
