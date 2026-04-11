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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network|Structure")
	TArray<int32> HiddenLayerSizes = { 16, 16 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network|Inputs")
	float MaxDistanceNormalization = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network|Inputs")
	TArray<float> FutureDotProductDistances = { 500.0f, 1000.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network|Inputs")
	float MaxVelocityNormalization = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network|Inputs")
	int32 RecurrentInputCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network|Outputs")
	int32 VehicleOutputCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Network", meta = (Units = "ms"))
	float NetworkUpdateFrequencyMS = 20.0f;

	/** Helper to calculate layer descriptors based on current settings */
	UFUNCTION(BlueprintCallable, Category = "Neural Network")
	TArray<FNeuralNetworkLayerDescriptor> GetNNLayerDescriptors() const;

	/** Total inputs required based on current configuration */
	UFUNCTION(BlueprintCallable, Category = "Neural Network")
	int32 GetTotalInputCount() const;

	/** Total outputs required based on current configuration (including recurrence) */
	UFUNCTION(BlueprintCallable, Category = "Neural Network")
	int32 GetTotalOutputCount() const;
};
