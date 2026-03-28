//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "VehicleTrainerContext.h"

TEST_CLASS(SplineCircuitTrainer_VehicleTrainerContext_Tests, "SplineCircuitTrainer.VehicleTrainerContext")
{
	TEST_METHOD(CanCreateVehicleTrainerContext)
	{
		AVehicleTrainerContext* Trainer = NewObject<AVehicleTrainerContext>();
		ASSERT_THAT(IsNotNull(Trainer));
	}
};
