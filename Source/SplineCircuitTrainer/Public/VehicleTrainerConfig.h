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

	//Number solutions in a population (ie solutions that can breed with each other)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	int32 Population = 10;

	//Number of populations of solutions (each num is an "island")
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
	
	// ----- Selection -----
	
	/** Number of elite solutions preserved per population (never replaced by worse performers) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Selection")
	int32 EliteCount = 2;

	/** Number of random candidates sampled per tournament when selecting parents for breeding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Selection", meta=(ClampMin="2"))
	int32 TournamentSize = 5;

	/** Probability that the best candidate wins a tournament (1.0 = always pick best, 0.0 = random) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Selection", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SelectionPressure = 0.85f;

	/** If true, any elite entity entering a tournament automatically wins over non-elites */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Selection")
	bool bElitesAlwaysWin = true;

	/** If true, higher fitness values are considered better (for minimization problems set to false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm")
	bool bHigherIsBetter = true;

	/** Per-parent chance to draw from the whole population instead of just the matching group when selecting parents (0.0 = same group only, 1.0 = always global) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Selection", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CrossGroupParentChance = 0.1f;

	// ----- Mutation -----

	/** Per-gene multiplicative noise magnitude: each weight is perturbed by ± this percentage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Mutation", meta=(ClampMin="0.0"))
	float PerValueDeltaPercent = 0.02f;

	/** Per-generation probability that a random hard-reset mutation occurs on a genome */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Mutation", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RandomMutationChance = 0.01f;

	/** Maximum fraction of genes that can be hard-reset when random mutation triggers (0.0-1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Mutation", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RandomResetMaxPercent = 0.025f;

	/** Minimum value used when hard-resetting a gene during random mutation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Mutation")
	float RandomResetMin = -1.0f;

	/** Maximum value used when hard-resetting a gene during random mutation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Mutation")
	float RandomResetMax = 1.0f;

	// ----- Breeding -----

	/** SBX crossover distribution index: higher values produce children closer to parents (less exploration) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Breeding", meta=(ClampMin="0.0"))
	float BreedingEta = 10.0f;

	/** Probability that SBX crossover is applied per gene (otherwise gene is copied directly from one parent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Breeding", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CrossoverProbability = 0.9f;

	/** If true, child gene values after crossover are clamped to [BreedingClampMin, BreedingClampMax] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Breeding")
	bool bClampChildren = true;

	/** Lower bound for clamping child gene values (used when bClampChildren is true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Breeding")
	float BreedingClampMin = -1.0f;

	/** Upper bound for clamping child gene values (used when bClampChildren is true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genetic Algorithm|Breeding")
	float BreedingClampMax = 1.0f;

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
