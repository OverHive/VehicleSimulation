// Fill out your copyright notice in the Description page of Project Settings.


#include "Tire.h"



UTire::UTire()
{
	WheelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMesh"));
	WheelMesh->SetupAttachment(this);
	WheelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WheelMesh->SetRelativeRotation(FRotator(0.0f, SteerAngle, 0.0f));

}

void UTire::UpdateTireRadius(const float Value)
{
	SuspensionSettings.WheelRadius = Value;
	if (WheelMeshDimension.X != 0 && WheelMeshDimension.Y != 0 && WheelMeshDimension.Z != 0 && WheelMesh)
	{
		FVector NewScale = FVector(
			Value / WheelMeshDimension.X,
			1,
			Value / WheelMeshDimension.Z);

		WheelMesh->SetRelativeScale3D(NewScale);
	}
}

void UTire::UpdateMaxGrip()
{
	MaxGrip = TireGrip * TireLoad;
}

float UTire::GetBrakingForce() const
{
	//Calculate the braking force from the torque based
	float BrakingForce = SuspensionSettings.WheelRadius != 0 ? BrakingTorque / SuspensionSettings.WheelRadius : 0;
	return GetTraction(BrakingForce);
}

float UTire::FrictionCircle(const float LongitudinalForce, const float LateralForce, const bool LongitudinalLeading) const
{
	float SelectedForce = LongitudinalLeading ? LateralForce : LongitudinalForce;
	//Get maximum force for selected force through a rearranged friction circle
	float MaxForce = FMath::Pow(MaxGrip, 2) - FMath::Pow(LongitudinalLeading ? LongitudinalForce : LateralForce, 2);
	//Return the result
	return MaxForce > 0 ? FMath::Sign(SelectedForce) * FMath::Min(FMath::Sqrt(MaxForce), FMath::Abs(SelectedForce)) : 0;

}

float UTire::GetLongitudinalForce(const float LongitudinalForceMagnitude, const float LateralForceMagnitude, const bool IsLongitudinalLeading, const bool ShouldUseFormula) const
{
	//Ignore for grounded wheels
	if (!IsGrounded)
	{
		return 0.0f;
	}
	//Select mode based on slip ratio(breaking mode for negative slip ratios, acceleration mode for positive slip)
	const int Mode = SlipRatio >= 0.0f ? WHEELMODE::ACCELERATION : WHEELMODE::BRAKING;
	//Calculate the force based on the Magic formula
	float RawLongitudinalForce = ShouldUseFormula ? MagicFormula(MaxGrip, FMath::Abs(SlipRatio), Mode) : LongitudinalForceMagnitude;
	RawLongitudinalForce = FMath::Abs(RawLongitudinalForce) * FMath::Sign(LongitudinalForceMagnitude);
	//Limit by friction circle
	return FrictionCircle(RawLongitudinalForce, LateralForceMagnitude, false);
}

FVector UTire::GetLateralForceVector() const
{
	if (!IsGrounded || FMath::Abs(SlipAngle) < KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	// Get the tire's local coordinate system
	FVector TireForward = GetForwardVector();
	FVector TireUp = GetUpVector();
	FVector TireRight = FVector::CrossProduct(TireUp, TireForward);

	// Calculate lateral force magnitude using Magic Formula
	float LateralForceMagnitude = MagicFormula(MaxGrip, FMath::Abs(SlipAngle), WHEELMODE::CORNERING);

	// Apply force opposite to slip direction
	float ForceDirection = -FMath::Sign(SlipAngle);

	return TireRight * (ForceDirection * LateralForceMagnitude);
}

void UTire::UpdateTireFrictionCoefficient(const float NewValue)
{
	TireFrictionCoefficient = NewValue;
	UpdateTotalGrip();
}

void UTire::UpdateFrictionCoefficient(const float NewValue)
{
	FrictionCoefficient = NewValue;
	UpdateTotalGrip();
}

void UTire::UpdateSteering(const float NewAngle)
{

	//Only allow steering from the front tires
	SteerAngle = IsFrontTire ? NewAngle : 0;
	//Rotate the tire to the new steer angle
	if (IsFrontTire)
	{
		SetRelativeRotation(FRotator(0.0f, SteerAngle, 0.0f));

		if (WheelMesh)
		{
			WheelMesh->SetRelativeRotation(FRotator(0.0f, SteerAngle, 0.0f));
		}
	}

}


void UTire::UpdateTireLoad(float NewWeight)
{
	TireLoad = FMath::Max(0.0f, NewWeight) + SuspensionSettings.UnSpringMass * Gravity;
	//Prevent negative tire load and tire load for aerial tires
	if (!IsGrounded)
	{
		TireLoad = 0;
	}
	//Update the maximum traction force on the tire with the new tire load
	UpdateMaxGrip();
}

void UTire::UpdateSlipRatio(const float VehicleSpeedAtWheel, const bool IsBraking)
{
	float WheelSurfaceSpeed = WheelRotationalVelocity * SuspensionSettings.WheelRadius;
	float Denominator = FMath::Abs(IsBraking ? VehicleSpeedAtWheel : WheelSurfaceSpeed);
	SlipRatio = FMath::Abs(Denominator) > 0.1f ? (WheelSurfaceSpeed - FMath::Abs(VehicleSpeedAtWheel)) / Denominator : 0;
}

void UTire::UpdateSlipAngle(const float LongitudinalVelocity, const float LateralVelocity)
{
	SlipAngle = FMath::Abs(LongitudinalVelocity) < 50.0f ? 0 : FMath::Atan2(LateralVelocity, LongitudinalVelocity);
}
void UTire::UpdateRollingRadius(FVector AxisPosition)
{
	RollingRadius = SuspensionSettings.WheelRadius;
	/*	SuspensionSettings.WheelRadius+ !ContactPoint.IsNearlyZero() && IsGrounded
		? FMath::Abs(AxisPosition.Z - ContactPoint.Z)
		: SuspensionSettings.WheelRadius;*/
}
void UTire::UpdateWheelFeatures(const TArray<float> NewStiffnessFactors, const TArray<float> NewShapeFactors, const TArray<float> NewCurvatureFactors)
{
	StiffnessFactors = NewStiffnessFactors;
	ShapeFactors = NewShapeFactors;
	CurvatureFactors = NewCurvatureFactors;
}

void UTire::ClampToVehicleWheelSpeed(const float WheelSpeed)
{
	float Radius = SuspensionSettings.WheelRadius;
	//If the tire is in the air skip
	if (!IsGrounded)
	{
		return;
	}

	WheelRotationalVelocity = Radius != 0 ? WheelSpeed / Radius : 0;
}

void UTire::StoreTireMeshDimensions()
{
	WheelMeshDimension = GetMeshDimensions(WheelMesh);
}

float UTire::GetRollingResistance() const
{
	return IsGrounded ? TireLoad * RollingResistanceCoefficient : 0;
}

float UTire::GetCompression(const float CurrentDistance)
{
	//Calculate the spring compression using the difference between the suspension 
	//length and the bottom of the wheel
	SuspensionCompression = FMath::Max(0.0f, SuspensionSettings.RestPosition + CurrentDistance);
	//Get the compression of the tire using by dividing the TireLoad by the tire Stiffness
	TireCompression = FMath::Max(0.0f, SuspensionSettings.TireVerticalStiffness != 0 ? TireLoad
		/ SuspensionSettings.TireVerticalStiffness : 0);
	return SuspensionCompression;
}


float UTire::CalculateSuspensionForce(const float SuspensionVelocity)
{
	float SpringForce = SuspensionCompression * SuspensionSettings.SpringStiffness;
	//Damping = suspension velocity* Damping coefficient
	float DampingForce = SuspensionVelocity * SuspensionSettings.DampingCoefficient;
	//The tire also acts like a spring when compressed due to its pressure
	float TireSpring = TireCompression * SuspensionSettings.TireVerticalStiffness;
	// Total Force = Spring force + Tire spring force  - Damping (Damping opposes the velocity)
	SuspensionForce = SpringForce + TireSpring + DampingForce;
	//Clamp the total force to prevent negative values 
	SuspensionForce = FMath::Max(0.0f, SuspensionForce);
	return IsGrounded ? SuspensionForce : 0;
}

float UTire::GetNormalForce() const
{
	return IsGrounded ? TireLoad : 0;
}

void UTire::UpdateWheelRotationalVelocity(const float DriveForce, const float TireForce, const float ResistiveForce, const float RotationSign, const float DeltaTime)
{
	float Radius = SuspensionSettings.WheelRadius;
	if (DeltaTime != 0)
	{
		//If the tire is in the air enter free rotation
		if (!IsGrounded)
		{
			WheelRotationalVelocity *= FMath::Pow(WheelDamper, DeltaTime);
		}

		if (Inertia != 0 && Radius != 0)
		{
			//Calculate the net force on the wheel
			const float NetForce = DriveForce - RotationSign * (TireForce + ResistiveForce);
			//Convert the force in to a torque
			float NetTorque = NetForce * Radius;
			//
			if (TireForce == 0 && DriveForce != 0)
			{
				NetTorque = Inertia;
			}
			//Calculate the acceleration
			float RotationalAcceleration = (NetTorque / Inertia) * DeltaTime;
			//Calculate the maximum speed change the wheel can undergo per time period
			const float MaxSurfaceSpeedChange = 0.05f *
				FMath::Max(FMath::Abs(WheelRotationalVelocity * Radius), 100.0f);
			//Clamp the acceleration based on the maximum velocity change
			RotationalAcceleration = FMath::Clamp(RotationalAcceleration,
				-MaxSurfaceSpeedChange / Radius, MaxSurfaceSpeedChange / Radius);
			//Prevent the wheel from swapping directions in a single frame
			RotationalAcceleration = RotationSign >= 0 ? FMath::Max(RotationalAcceleration, -WheelRotationalVelocity) : FMath::Min(RotationalAcceleration, -WheelRotationalVelocity);
			//Update the rotation velocity with the acceleration
			if (RotationalAcceleration != 0)
			{
				WheelRotationalVelocity += RotationalAcceleration;
			}
		}
	}
}

float UTire::MagicFormula(const float peakValue, const float x, const int Index) const
{

	int FactorIndex = FMath::Clamp(Index, 0, 2);
	float StiffnessEffect = StiffnessFactors[FactorIndex] * x;
	float CurvatureEffect = CurvatureFactors[FactorIndex] * (StiffnessEffect - FMath::Atan(StiffnessEffect));
	float arc = FMath::Atan(StiffnessEffect - CurvatureEffect);
	return peakValue * FMath::Sin(ShapeFactors[FactorIndex] * arc);
}

void UTire::StoreTireContactInformation(const FHitResult Hit)
{
	ContactPoint = Hit.Location;
	ContacNormal = Hit.Normal;
}

void UTire::UpdateSuspension(const float Stiffness, const float Damping, const float SuspensionLength, const float TireStiffness, const float UnSprungMass, const float StaticTireLoad)
{
	SuspensionSettings.SpringStiffness = Stiffness;

	SuspensionSettings.DampingCoefficient = Damping;

	SuspensionSettings.SuspensionLength = SuspensionLength;

	SuspensionSettings.TireVerticalStiffness = TireStiffness;

	SuspensionSettings.UnSpringMass = UnSprungMass;

	//Get the static force on the string 
	UpdateTireLoad(StaticTireLoad);
	float ForceOnSpring = TireLoad;

	//Calculate the resting suspension length

	SuspensionSettings.RestPosition = SuspensionSettings.SpringStiffness != 0 ? TireLoad / SuspensionSettings.SpringStiffness : 0;
	ResetForces();

}

UTire::~UTire()
{
}

float UTire::GetTraction(const float ThrottleForce) const
{
	return IsFrontTire && IsGrounded ?
		FMath::Clamp(ThrottleForce, -MaxGrip, MaxGrip)
		: 0;
}
