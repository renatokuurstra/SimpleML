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
	float SpawnVerticalOffset = 0.0f;

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

	// Genetic Algorithm Settings
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	int32 EliteCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	int32 TournamentSize = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float SelectionPressure = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float MutationRate = 0.05f;

	/** Multiplicative delta for mutation: ± this percent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float PerValueDeltaPercent = 0.02f;

	/** Chance to trigger a random reset mutation on a value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float RandomMutationChance = 0.01f;

	/** Breeding eta parameter (crossover) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float BreedingEta = 10.0f;

	/** Percentage of population (worst performers) to reset each generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float BottomResetFraction = 0.2f;

	/** Threshold distance from spline to trigger a reset (e.g. out of bounds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float MaxSplineDistanceThreshold = 5000.0f;

	/** Max time in seconds allowed without positive progress along the spline */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float NoProgressTimeout = 2.0f;

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
