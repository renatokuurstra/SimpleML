//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "Systems/EliteSelectionFloatSystem.h"
#include "VehicleEliteSelectionSystem.generated.h"

/**
 * Vehicle-specific elite selection system.
 * Overrides base hooks to provide actor names and vehicle locations for debug visualization.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class SPLINECIRCUITTRAINER_API UVehicleEliteSelectionSystem : public UEliteSelectionFloatSystem
{
	GENERATED_BODY()

protected:
	virtual FString GetSourceEntityLabel(entt::entity E) const override;
	virtual FVector GetSourceEntityLocation(entt::entity E) const override;
};
