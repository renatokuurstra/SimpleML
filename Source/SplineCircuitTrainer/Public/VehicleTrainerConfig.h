//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NeuralNetwork.h"
#include "VehicleTrainerConfig.generated.h"

USTRUCT(BlueprintType)
struct SPLINECIRCUITTRAINER_API FResetReasonConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset", meta = (GetOptions = "GetResetReasonOptions"))
	FName Reason;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset")
	bool bBlockBreed = false;
};

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
	float SpawnParametricDistance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	int32 Population = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	int32 NumPopulations = 1;

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Selection")
	int32 EliteCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Selection")
	int32 TournamentSize = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Selection")
	float SelectionPressure = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	bool bHigherIsBetter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float MutationRate = 0.05f;

	/** Multiplicative delta for mutation: ± this percent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float PerValueDeltaPercent = 0.02f;

	/** Chance to trigger a random reset mutation on a value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	float RandomMutationChance = 0.01f;

	/** Breeding eta parameter (crossover) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Breeding")
	float BreedingEta = 10.0f;

	/** Percentage of population (worst performers) to reset each generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Reset Logic")
	float BottomResetFraction = 0.2f;

	/** Threshold distance from spline to trigger a reset (e.g. out of bounds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Reset Logic")
	float MaxSplineDistanceThreshold = 5000.0f;

	/** Max time in seconds allowed without positive progress along the spline */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Reset Logic")
	float NoProgressTimeout = 2.0f;

	/** Minimum progress in centimeters required between evaluations to avoid reset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Reset Logic")
	float MinimumProgressBetweenEvaluations = 10.0f;

	/** Minimum average velocity (cm/s) to maintain to avoid reset. Only checked after MinAgeForReset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Reset Logic")
	float MinAverageVelocity = 100.0f;

	/** Minimum age in seconds before average velocity checks start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Reset Logic")
	float MinAgeForReset = 2.0f;

	/** Minimum age in seconds before an entity is eligible for fitness scoring and breeding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Fitness Eligibility")
	float MinBreedAge = 10.0f;

	/** Percentage (0.0 - 1.0) of the highest fitness overall required for fitness eligibility. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Fitness Eligibility")
	float HighestFitnessFactor = 0.5f;

	/** Configuration for each reset reason, allowing to block breeding if necessary. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Reset Logic")
	TArray<FResetReasonConfig> ResetReasonConfigs;

	/** Helper to calculate layer descriptors based on current settings */
	UFUNCTION(BlueprintCallable, Category = "Neural Network")
	TArray<FNeuralNetworkLayerDescriptor> GetNNLayerDescriptors() const;

	/** Total inputs required based on current configuration */
	UFUNCTION(BlueprintCallable, Category = "Neural Network")
	int32 GetTotalInputCount() const;

	/** Total outputs required based on current configuration (including recurrence) */
	UFUNCTION(BlueprintCallable, Category = "Neural Network")
	int32 GetTotalOutputCount() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDebugInfo = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	int32 FitnessHistoryLength = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float DebugLogFrequency = 1.0f;

	/** Global toggle for the nuke system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Nuke")
	bool bEnableNuke = true;

	/** The minimum relative improvement required to consider a context "active." */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Nuke")
	float StalenessThreshold = 0.01f;

	/** Time to wait after a nuke before evaluating staleness again for that context. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Nuke")
	float StalenessCooldown = 50.0f;

	/** Minimum number of historical values required before staleness check. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Nuke")
	int32 MinHistoryForStaleness = 50;

	UFUNCTION(BlueprintCallable, Category = "Genetic Algorithm")
	TArray<FName> GetResetReasonOptions() const;
};
