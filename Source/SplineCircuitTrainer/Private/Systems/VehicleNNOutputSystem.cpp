//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleNNOutputSystem.h"
#include "VehicleComponent.h"
#include "Components/NNIOComponents.h"
#include "VehicleNNInterface.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "GameFramework/Pawn.h"
#include "UObject/Interface.h"

UVehicleNNOutputSystem::UVehicleNNOutputSystem()
{
	RegisterComponent<FVehicleComponent>();
	RegisterComponent<FNNOutFloatComp>();
}

void UVehicleNNOutputSystem::Update_Implementation(float DeltaTime)
{
	AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
	if (!TrainerContext || !TrainerContext->TrainerConfig)
	{
		return;
	}

	auto View = GetView<FVehicleComponent, FNNOutFloatComp>();

	const int32 OutputCount = TrainerContext->TrainerConfig->VehicleOutputCount;

	for (auto Entity : View)
	{
		const FVehicleComponent& VehicleComp = View.get<FVehicleComponent>(Entity);
		const FNNOutFloatComp& OutComp = View.get<FNNOutFloatComp>(Entity);

		if (VehicleComp.VehiclePawn && VehicleComp.VehiclePawn->GetClass()->ImplementsInterface(UVehicleNNInterface::StaticClass()))
		{
			if (OutComp.Values.Num() >= OutputCount)
			{
				// Only pass the first N elements (ignoring recurrence outputs)
				TArrayView<const float> OutputView(OutComp.Values.GetData(), OutputCount);
				Cast<IVehicleNNInterface>(VehicleComp.VehiclePawn)->ApplyNNOutputs(OutputView);
			}
		}
	}
}
