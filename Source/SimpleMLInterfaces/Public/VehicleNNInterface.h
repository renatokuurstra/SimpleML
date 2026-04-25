// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VehicleNNInterface.generated.h"

/**
 * Interface for vehicles that provide neural network input data.
 *
 * NOTE: Steering/throttle/brake are NOT included here because the recurrent
 * neurons already feed the NN's previous output back into its input — the
 * network inherently knows what it commanded last frame.
 *
 * What IS included: internal engine/physics state the NN cannot derive from
 * its own outputs (RPM, gear, per-wheel physics).
 */
UINTERFACE(MinimalAPI)
class UVehicleNNInterface : public UInterface
{
	GENERATED_BODY()
};

class SIMPLEMLINTERFACES_API IVehicleNNInterface
{
	GENERATED_BODY()

public:
	virtual void ApplyNNOutputs(TArrayView<const float> Outputs) = 0;

	virtual float GetWheelsOnGroundRatio() const { return 1.0f; }

	/**
	 * Per-wheel ground contact: 1.0 = firmly on ground, 0.0 = in air.
	 * One entry per wheel (typically 4).
	 */
	virtual void GetWheelContactStates(TArray<float>& OutStates) const { OutStates.Reset(); }

	/**
	 * Per-wheel suspension compression: 0.0 = fully extended, 1.0 = fully compressed.
	 * One entry per wheel (typically 4).
	 */
	virtual void GetWheelSuspensionCompression(TArray<float>& OutCompression) const { OutCompression.Reset(); }

	/**
	 * Normalized engine RPM: 0.0 = idle, 1.0 = redline.
	 */
	virtual float GetNormalizedRPM() const { return 0.0f; }

	/**
	 * Normalized current gear: 0.0 = reverse/idle, 1.0 = top gear.
	 * MaxGear is the total number of forward gears in the transmission.
	 */
	virtual float GetNormalizedGear(int32 MaxGear) const { return 0.5f; }
};