//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "UI/VehicleTrainerDebugWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UVehicleTrainerDebugWidget::DrawLineGraph(FPaintContext& Context, FVector2D LocalSize, const TArray<float>& Values, FLinearColor Color, float Thickness)
{
	if (Values.Num() < 2)
	{
		return;
	}

	float MaxVal = -MAX_FLT;
	float MinVal = MAX_FLT;

	for (float Val : Values)
	{
		MaxVal = FMath::Max(MaxVal, Val);
		MinVal = FMath::Min(MinVal, Val);
	}

	float Range = MaxVal - MinVal;
	if (Range <= 0.0f)
	{
		Range = 1.0f;
	}

	float XStep = LocalSize.X / (Values.Num() - 1);

	for (int32 i = 0; i < Values.Num() - 1; ++i)
	{
		FVector2D StartPos(i * XStep, LocalSize.Y - ((Values[i] - MinVal) / Range) * LocalSize.Y);
		FVector2D EndPos((i + 1) * XStep, LocalSize.Y - ((Values[i + 1] - MinVal) / Range) * LocalSize.Y);

		UWidgetBlueprintLibrary::DrawLine(Context, StartPos, EndPos, Color, true, Thickness);
	}
}
