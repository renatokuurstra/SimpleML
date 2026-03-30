//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleNNOutputSystem.h"
#include "VehicleComponent.h"
#include "Components/NNIOComponents.h"
#include "VehicleNNInterface.h"
#include "GameFramework/Pawn.h"
#include "UObject/Interface.h"

UVehicleNNOutputSystem::UVehicleNNOutputSystem()
{
	RegisterComponent<FVehicleComponent>();
	RegisterComponent<FNNOutFloatComp>();
}

void UVehicleNNOutputSystem::Update(float DeltaTime)
{
	auto View = GetView<FVehicleComponent, FNNOutFloatComp>();

	for (auto Entity : View)
	{
		const FVehicleComponent& VehicleComp = View.get<FVehicleComponent>(Entity);
		const FNNOutFloatComp& OutComp = View.get<FNNOutFloatComp>(Entity);

		if (VehicleComp.VehiclePawn && VehicleComp.VehiclePawn->GetClass()->ImplementsInterface(USimpleMLVehicleNNInterface::StaticClass()))
		{
			Cast<ISimpleMLVehicleNNInterface>(VehicleComp.VehiclePawn)->ApplyNNOutputs(OutComp.Values);
		}
	}
}
