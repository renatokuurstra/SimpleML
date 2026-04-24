//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleEliteSelectionSystem.h"
#include "VehicleComponent.h"

FString UVehicleEliteSelectionSystem::GetSourceEntityLabel(entt::entity E) const
{
	auto& Registry = GetRegistry();
	
	if (!Registry.all_of<FVehicleComponent>(E))
	{
		return TEXT("Unknown");
	}
	
	const FVehicleComponent& Vehicle = Registry.get<FVehicleComponent>(E);
	if (!Vehicle.VehiclePawn)
	{
		return TEXT("Unknown");
	}
	
	return Vehicle.VehiclePawn->GetName();
}

FVector UVehicleEliteSelectionSystem::GetSourceEntityLocation(entt::entity E) const
{
	auto& Registry = GetRegistry();
	
	if (!Registry.all_of<FVehicleComponent>(E))
	{
		return FVector::ZeroVector;
	}
	
	const FVehicleComponent& Vehicle = Registry.get<FVehicleComponent>(E);
	if (!Vehicle.VehiclePawn)
	{
		return FVector::ZeroVector;
	}
	
	return Vehicle.VehiclePawn->GetActorLocation();
}
