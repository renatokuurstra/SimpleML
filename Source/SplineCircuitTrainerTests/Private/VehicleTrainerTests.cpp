//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "VehicleTrainer.h"

TEST_CLASS(SplineCircuitTrainer_VehicleTrainer_Tests, "SplineCircuitTrainer.VehicleTrainer")
{
	TEST_METHOD(CanCreateVehicleTrainer)
	{
		AVehicleTrainer* Trainer = NewObject<AVehicleTrainer>();
		ASSERT_THAT(IsNotNull(Trainer));
	}
};
