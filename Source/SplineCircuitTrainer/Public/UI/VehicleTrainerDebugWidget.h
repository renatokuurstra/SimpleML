//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/GenomeComponents.h"
#include "VehicleTrainerDebugWidget.generated.h"

USTRUCT(BlueprintType)
struct FSplineCircuitAliveEntityUIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Trainer")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Trainer")
	float Fitness = 0.0f;
};

/**
 * Data structure to pass to the widget representing a single trainer context.
 */
USTRUCT(BlueprintType)
struct FVehicleTrainerContextUIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Trainer")
	FString ContextName;

	UPROPERTY(BlueprintReadOnly, Category = "Trainer")
	TArray<float> HistoricalTotalEliteFitness;

	UPROPERTY(BlueprintReadOnly, Category = "Trainer")
	TArray<FSplineCircuitAliveEntityUIData> AliveEntities;

	UPROPERTY(BlueprintReadOnly, Category = "Trainer")
	int32 ResetCount = 0;
};

/**
 * UVehicleTrainerDebugWidget
 * Base class for the debug UI widget.
 */
UCLASS(Abstract)
class SPLINECIRCUITTRAINER_API UVehicleTrainerDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Updates the widget with data from all contexts */
	UFUNCTION(BlueprintImplementableEvent, Category = "Trainer")
	void UpdateContextData(const TArray<FVehicleTrainerContextUIData>& ContextData);

	/** Helper to visualize a line graph from a series of floats */
	UFUNCTION(BlueprintCallable, Category = "Trainer")
	static void DrawLineGraph(UPARAM(ref) FPaintContext& Context, FVector2D LocalSize, const TArray<float>& Values, FLinearColor Color, float Thickness = 1.0f);
};
