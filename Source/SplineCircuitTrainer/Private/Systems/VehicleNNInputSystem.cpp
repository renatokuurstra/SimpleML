//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

/**
 * VehicleNNInputSystem - Collects vehicle state for neural network inputs.
 *
 * Input Layout (all normalized to [-1, 1] or [0, 1]):
 *
 *  Spline Position (9 inputs):
 *   0: Signed distance to spline [-1,1]
 *   1: Current orientation Z-cross product [-1,1]
 *   2,3: Future orientations at 500cm, 1000cm [-1,1]
 *   4: Height above/below spline [-1,1]
 *   5: Spline curvature ahead [0,1]
 *   6: Spline curvature direction (signed left/right) [-1,1]
 *   7: Spline progress (normalized 0-1 along circuit) [0,1]
 *   8: Forward velocity along spline tangent [-1,1]
 *
 *  Vehicle Dynamics (6 inputs):
 *   9: Velocity magnitude [0,1]
 *  10: Pitch angle [-1,1]
 *  11: Roll angle [-1,1]
 *  12: Lateral velocity (perpendicular to spline) [-1,1]
 *  13: Vertical velocity [-1,1]
 *  14: Angular velocity pitch rate [-1,1]
 *  15: Angular velocity yaw rate [-1,1]
 *  16: Angular velocity roll rate [-1,1]
 *
 *  Wheels (9 inputs):
 *  17: Wheels on ground ratio [0,1]
 *  18-21: Per-wheel contact state (1.0=ground, 0.0=air) [0,1]
 *  22-25: Per-wheel suspension compression [0,1]
 *
 *  Engine (2 inputs):
 *  26: Normalized engine RPM [0,1]
 *  27: Normalized current gear [0,1]
 *
 *  NOTE: Steering/throttle/brake are NOT included - the recurrent neurons
 *  already feed the NN's previous output back into its input, so the network
 *  inherently knows what it commanded last frame.
 *
 *  Total: 28 base inputs + recurrent neurons
 */

#include "Systems/VehicleNNInputSystem.h"
#include "VehicleComponent.h"
#include "Components/NNIOComponents.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "VehicleNNInterface.h"
#include "Components/SplineComponent.h"

UVehicleNNInputSystem::UVehicleNNInputSystem()
{
	RegisterComponent<FVehicleComponent>();
	RegisterComponent<FNNInFLoatComp>();
}

void UVehicleNNInputSystem::Update_Implementation(float DeltaTime)
{
	AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
	if (!TrainerContext || !TrainerContext->TrainerConfig)
	{
		return;
	}

	USplineComponent* Spline = TrainerContext->GetCircuitSpline();
	if (!Spline)
	{
		return;
	}

	const UVehicleTrainerConfig& Config = *TrainerContext->TrainerConfig;

	// Cache spline properties
	const float SplineLength = Spline->GetSplineLength();
	if (SplineLength <= 0.0f)
	{
		return;
	}

	// Spline up vector (default up vector from spline component)
	const FVector SplineUpVector = Spline->GetDefaultUpVector(ESplineCoordinateSpace::World) != FVector::ZeroVector
		? Spline->GetDefaultUpVector(ESplineCoordinateSpace::World)
		: FVector::UpVector;

	// Normalization thresholds from config
	const float MaxVelocityNorm = Config.MaxVelocityNormalization;
	const float MaxForwardVelocity = Config.MaxForwardVelocity;
	const float MaxVerticalVelocity = Config.MaxVerticalVelocity;
	const float MaxAngularVelocity = Config.MaxAngularVelocityDegPerSec;
	const float MaxDistance = Config.MaxDistanceNormalization;
	const float MaxHeight = Config.MaxHeightDifferenceCM;
	const float MaxPitchRoll = Config.MaxPitchRollRadians;
	const float LookaheadDistance = Config.CurvatureLookaheadDistance;

	const int32 TotalInputCount = Config.GetTotalInputCount();

	const TArray<float>& FutureDistances = Config.FutureDotProductDistances;

	auto View = GetView<FVehicleComponent, FNNInFLoatComp>();

	for (auto Entity : View)
	{
		const FVehicleComponent& VehicleComp = View.get<FVehicleComponent>(Entity);
		FNNInFLoatComp& InComp = View.get<FNNInFLoatComp>(Entity);

		if (!VehicleComp.VehiclePawn)
		{
			continue;
		}

		APawn* Pawn = VehicleComp.VehiclePawn;

		// Ensure input array is correctly sized
		InComp.Values.SetNumZeroed(TotalInputCount);

		// Get vehicle interface for wheel/engine data
		const bool bImplementsInterface = Pawn->GetClass()->ImplementsInterface(UVehicleNNInterface::StaticClass());
		const IVehicleNNInterface* VehicleInterface = bImplementsInterface ? Cast<IVehicleNNInterface>(Pawn) : nullptr;

		// Actor transform and velocity
		const FVector ActorLocation = Pawn->GetActorLocation();
		const FRotator ActorRotation = Pawn->GetActorRotation();
		const FVector ActorVelocity = Pawn->GetVelocity();

		// Find closest point on spline (input key, not distance)
		const float CurrentInputKey = Spline->FindInputKeyClosestToWorldLocation(ActorLocation);
		const FVector SplineLocation = Spline->GetLocationAtSplineInputKey(CurrentInputKey, ESplineCoordinateSpace::World);

		// Spline tangent at closest point
		FVector SplineTangent = Spline->GetDirectionAtSplineInputKey(CurrentInputKey, ESplineCoordinateSpace::World);
		if (SplineTangent.IsNearlyZero())
		{
			SplineTangent = Spline->GetDirectionAtSplineInputKey(0, ESplineCoordinateSpace::World);
		}

		// Distance along spline for lookahead calculations
		const float CurrentSplineDistance = Spline->GetDistanceAlongSplineAtLocation(ActorLocation, ESplineCoordinateSpace::World);

		// Vector from spline to actor
		const FVector ToActorVector = ActorLocation - SplineLocation;

		// Spline right vector (perpendicular to tangent in horizontal plane)
		const FVector SplineRightVector = FVector::CrossProduct(SplineTangent, SplineUpVector).GetSafeNormal();

		// --- Calculate inputs ---

		// Signed distance to spline
		const float SignedDistance = FVector::DotProduct(ToActorVector, SplineRightVector);

		// Height above/below spline
		const float HeightAboveSpline = FVector::DotProduct(ToActorVector, SplineUpVector);

		// Current orientation - Z cross product
		const FVector ActorForward = ActorRotation.Vector();
		const float CurrentOrientation = FVector::DotProduct(FVector::CrossProduct(SplineTangent, ActorForward), SplineUpVector);

		// Future orientation dot products (use distance-based lookahead, not input key + distance)
		TArray<float> FutureOrientations;
		FutureOrientations.SetNum(FutureDistances.Num());
		for (int32 i = 0; i < FutureDistances.Num(); ++i)
		{
			const float LookaheadDistanceAlongSpline = FMath::Clamp(CurrentSplineDistance + FutureDistances[i], 0.0f, SplineLength);
			const float LookaheadInputKey = Spline->FindInputKeyClosestToWorldLocation(
				Spline->GetLocationAtDistanceAlongSpline(LookaheadDistanceAlongSpline, ESplineCoordinateSpace::World));
			const FVector FutureTangent = Spline->GetDirectionAtSplineInputKey(LookaheadInputKey, ESplineCoordinateSpace::World);
			FutureOrientations[i] = FVector::DotProduct(FVector::CrossProduct(FutureTangent, ActorForward), SplineUpVector);
		}

		// Velocity magnitude
		const float VelocityMagnitude = ActorVelocity.Size();

		// Pitch and roll angles (convert degrees to radians for normalization)
		const float PitchAngleRad = FMath::DegreesToRadians(ActorRotation.Pitch);
		const float RollAngleRad = FMath::DegreesToRadians(ActorRotation.Roll);

		// Angular velocity from root primitive component (in degrees/s)
		const FVector AngularVelocity = Cast<UPrimitiveComponent>(Pawn->GetRootComponent())
			? Cast<UPrimitiveComponent>(Pawn->GetRootComponent())->GetPhysicsAngularVelocityInDegrees()
			: FVector::ZeroVector;

		// Forward velocity along spline tangent
		const float ForwardVelocityAlongSpline = FVector::DotProduct(ActorVelocity, SplineTangent);

		// Lateral velocity (perpendicular to spline)
		const float LateralVelocity = FVector::DotProduct(ActorVelocity, SplineRightVector);

		// Vertical velocity
		const float VerticalVelocity = FVector::DotProduct(ActorVelocity, SplineUpVector);

		// Spline curvature ahead (use distance-based lookahead, not input key + distance)
		const float CurvatureLookaheadDistance = FMath::Clamp(CurrentSplineDistance + LookaheadDistance, 0.0f, SplineLength);
		const float CurvatureLookaheadKey = Spline->FindInputKeyClosestToWorldLocation(
			Spline->GetLocationAtDistanceAlongSpline(CurvatureLookaheadDistance, ESplineCoordinateSpace::World));
		const FVector CurvatureTangent = Spline->GetDirectionAtSplineInputKey(CurvatureLookaheadKey, ESplineCoordinateSpace::World);
		const float Curvature = 1.0f - FMath::Clamp(FVector::DotProduct(SplineTangent, CurvatureTangent), 0.0f, 1.0f);

		// Spline curvature direction (signed)
		const float CurvatureDirection = FVector::DotProduct(FVector::CrossProduct(SplineTangent, CurvatureTangent), SplineUpVector);

		// Spline progress (normalized 0-1 along circuit)
		const float SplineProgress = CurrentSplineDistance / SplineLength;

		// Engine state inputs from vehicle interface
		float NormalizedRPM = 0.0f;
		float NormalizedGear = 0.5f;
		if (VehicleInterface)
		{
			NormalizedRPM = VehicleInterface->GetNormalizedRPM();
			NormalizedGear = VehicleInterface->GetNormalizedGear(6);
		}

		// Per-wheel inputs from vehicle interface
		TArray<float> WheelContactStates;
		TArray<float> WheelSuspensionCompression;
		if (VehicleInterface)
		{
			VehicleInterface->GetWheelContactStates(WheelContactStates);
			VehicleInterface->GetWheelSuspensionCompression(WheelSuspensionCompression);
		}

		// Wheels on ground ratio
		const float WheelsOnGroundRatio = VehicleInterface ? VehicleInterface->GetWheelsOnGroundRatio() : 1.0f;

		// --- Write normalized inputs ---
		int32 InputIndex = 0;

		// --- Spline Position (9 inputs) ---

		// 0: Signed distance to spline [-1,1]
		InComp.Values[InputIndex++] = FMath::Clamp(SignedDistance / MaxDistance, -1.0f, 1.0f);

		// 1: Current orientation Z-cross product [-1,1]
		InComp.Values[InputIndex++] = FMath::Clamp(CurrentOrientation, -1.0f, 1.0f);

		// 2,3: Future orientations [-1,1]
		for (const float Orientation : FutureOrientations)
		{
			InComp.Values[InputIndex++] = FMath::Clamp(Orientation, -1.0f, 1.0f);
		}

		// 4: Height above/below spline [-1,1]
		InComp.Values[InputIndex++] = FMath::Clamp(HeightAboveSpline / MaxHeight, -1.0f, 1.0f);

		// 5: Spline curvature ahead [0,1]
		InComp.Values[InputIndex++] = FMath::Clamp(Curvature, 0.0f, 1.0f);

		// 6: Spline curvature direction (signed) [-1,1]
		InComp.Values[InputIndex++] = FMath::Clamp(CurvatureDirection, -1.0f, 1.0f);

		// 7: Spline progress (normalized 0-1) [0,1]
		InComp.Values[InputIndex++] = FMath::Clamp(SplineProgress, 0.0f, 1.0f);

		// 8: Forward velocity along spline tangent [-1,1]
		InComp.Values[InputIndex++] = FMath::Clamp(ForwardVelocityAlongSpline / MaxForwardVelocity, -1.0f, 1.0f);

		// --- Vehicle Dynamics (6 inputs) ---

		// 9: Velocity magnitude [0,1]
		InComp.Values[InputIndex++] = FMath::Clamp(VelocityMagnitude / MaxVelocityNorm, 0.0f, 1.0f);

		// 10: Pitch angle [-1,1] (radians)
		InComp.Values[InputIndex++] = FMath::Clamp(PitchAngleRad / MaxPitchRoll, -1.0f, 1.0f);

		// 11: Roll angle [-1,1] (radians)
		InComp.Values[InputIndex++] = FMath::Clamp(RollAngleRad / MaxPitchRoll, -1.0f, 1.0f);

		// 12: Lateral velocity (perpendicular to spline) [-1,1]
		InComp.Values[InputIndex++] = FMath::Clamp(LateralVelocity / MaxVelocityNorm, -1.0f, 1.0f);

		// 13: Vertical velocity [-1,1]
		InComp.Values[InputIndex++] = FMath::Clamp(VerticalVelocity / MaxVerticalVelocity, -1.0f, 1.0f);

		// 14,15,16: Angular velocity (pitch/yaw/roll rates) [-1,1]
		InComp.Values[InputIndex++] = FMath::Clamp(AngularVelocity.X / MaxAngularVelocity, -1.0f, 1.0f);
		InComp.Values[InputIndex++] = FMath::Clamp(AngularVelocity.Y / MaxAngularVelocity, -1.0f, 1.0f);
		InComp.Values[InputIndex++] = FMath::Clamp(AngularVelocity.Z / MaxAngularVelocity, -1.0f, 1.0f);

		// --- Wheels (9 inputs) ---

		// 17: Wheels on ground ratio [0,1]
		InComp.Values[InputIndex++] = FMath::Clamp(WheelsOnGroundRatio, 0.0f, 1.0f);

		// 18-21: Per-wheel contact states [0,1]
		for (int32 i = 0; i < 4 && i < WheelContactStates.Num(); ++i)
		{
			InComp.Values[InputIndex++] = WheelContactStates[i];
		}
		while (InputIndex < 22)
		{
			InComp.Values[InputIndex++] = 0.0f;
		}

		// 22-25: Per-wheel suspension compression [0,1]
		for (int32 i = 0; i < 4 && i < WheelSuspensionCompression.Num(); ++i)
		{
			InComp.Values[InputIndex++] = WheelSuspensionCompression[i];
		}
		while (InputIndex < 26)
		{
			InComp.Values[InputIndex++] = 0.0f;
		}

		// --- Engine (2 inputs) ---

		// 26: Normalized engine RPM [0,1]
		InComp.Values[InputIndex++] = FMath::Clamp(NormalizedRPM, 0.0f, 1.0f);

		// 27: Normalized current gear [0,1]
		InComp.Values[InputIndex++] = FMath::Clamp(NormalizedGear, 0.0f, 1.0f);
	}
}
