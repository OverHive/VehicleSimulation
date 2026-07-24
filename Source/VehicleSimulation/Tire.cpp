// Fill out your copyright notice in the Description page of Project Settings.


#include "Tire.h"


UTire::UTire()
{
}

void UTire::UpdateMaxTraction()
{
	MaximumWheelTraction = FrictionCoefficient * TireLoad;
}

const float UTire::GetLateralGrip()
{
	return IsGrounded ? FrictionCoefficient * TireLoad : 0;
}

void UTire::UpdateSteering(const float NewAngle)
{

	//Only allow steering from the front tires
	SteerAngle = IsFrontTire ? NewAngle : 0;
	//Rotate the tire to the new steer angle
	if (IsFrontTire)
		SetRelativeRotation(FRotator(0.0f, SteerAngle, 0.0f));

}


void UTire::UpdateTireLoad(float NewWeight)
{
	TireLoad = FMath::Max(0.0f, NewWeight);
	//Prevent negative tire load and tire load for aerial tires
	if (!IsGrounded)
	{
		TireLoad = 0;
	}
	//Update the maximum traction force on the tire with the new tire load
	UpdateMaxTraction();
}

float UTire::CalculateWheelRotationalVelocity(const float VehicleSpeed)
{
	//If the tire is in the air enter free rotation
	if (!IsGrounded)
	{
		WheelRotationalVelocity *= WheelDamper;
		return 0;
	}
	//Calculate the rotational velocity of the wheel
	float TargetRotationalVelocity = VehicleSpeed / (SuspensionSettings.WheelRadius / 100.0f);
	//
	WheelRotationalVelocity = FMath::Lerp(WheelRotationalVelocity, TargetRotationalVelocity, CouplingFactor);
	return WheelRotationalVelocity;
}

float UTire::CalculateSlipRatio(const float VehicleSpeedAtWheel)
{
	float WheelSurfaceSpeed = GetRotationalVelocity();
	SlipRatio = FMath::Abs(VehicleSpeedAtWheel) > 0.1f ?(WheelSurfaceSpeed - VehicleSpeedAtWheel) / FMath::Max(FMath::Abs(VehicleSpeedAtWheel), FMath::Abs(WheelSurfaceSpeed))
		: 0;
	return SlipRatio;
}

float UTire::CalculateSlipAngle(const float VelocityY, const float VelocityX)
{
	SlipAngle = VelocityX != 0 ? FMath::Atan2(VelocityY , VelocityX) : 0;
	return SlipAngle;
}

void UTire::ApplyBrakes(const float AppliedBrakeTorque, const float DeltaTime)
{
	//Calculate the deceleration
	float AngularDeceleration = -AppliedBrakeTorque / WheelRotationalInertia;

	// Get current rotation direction and apply deceleration
	float CurrentDirection = FMath::Sign(WheelRotationalVelocity);
	float NewVelocity = FMath::Abs(WheelRotationalVelocity) - AngularDeceleration * DeltaTime;

	//Update the rotational velocity, stopping at zero to prevent negative rotation
	WheelRotationalVelocity = FMath::Max(0.0f, NewVelocity) * CurrentDirection;
}


float UTire::GetRollingResistance() const
{
	return TireLoad * RollingResistanceCoefficient;
}

float UTire::GetCompression(const float CurrentDistance)
{
	//Calculate the spring compression using the difference between the suspension length and the bottom of the wheel
	// and clamp it to prevent negative values/extreme forces
	SuspensionCompression = FMath::Max(0.0f, SuspensionSettings.SuspensionLength + SuspensionSettings.WheelRadius - CurrentDistance);
	return SuspensionCompression;
}


float UTire::CalculateSuspensionForce(const float SuspensionVelocity)
{
	float SpringForce = SuspensionCompression * SuspensionSettings.SpringStiffness;
	//Damping = suspension velocity* Damping coefficient
	float DampingForce = SuspensionVelocity * SuspensionSettings.DampingCoefficient;
	// Total Force = Spring - Damping (Damping opposes the velocity)
	SuspensionForce = SpringForce - DampingForce;
	//Clamp the total force to prevent negative values 
	SuspensionForce = FMath::Max(0.0f, SuspensionForce);
	return SuspensionForce;
}

float UTire::MagicFormula(const float peakValue, const float x) const
{
	float StiffnessEffect = StiffnessFactor * x;
	float CurvatureEffect = CurvatureFactor * (StiffnessEffect - FMath::Atan(StiffnessEffect));
	float arc = FMath::Atan(StiffnessEffect - CurvatureEffect);
	return peakValue *sin(ShapeFactor*FMath::Atan(StiffnessEffect-CurvatureEffect));
}

void UTire::UpdateVehicleParameters(const float mass, const float wheelBaseLength, const float trackWidth, const float DistanceOfCGToFrontAxis,
	const float DistanceOfCGToRearAxis, const float CGHeight, const float NewGravity)
{
	Gravity = NewGravity;
	VehicleMass = mass;
	VehicleWeight = mass * Gravity;
	BaseTireLoad = VehicleWeight / 4;
	WheelBase = wheelBaseLength;
	TrackWidth = trackWidth;
	DistanceOfCentreOfGravityToTireAxis = IsFrontTire ? DistanceOfCGToFrontAxis : DistanceOfCGToRearAxis;
	CentreOfGravityHeight = CGHeight;

}

void UTire::StoreTireContactLocation(const FVector NewHitLocation)
{
	ContactPoint = NewHitLocation;
}

void UTire::UpdateSuspension(const float Stiffness, const float Damping, const float SuspensionLength)
{
	SuspensionSettings.SpringStiffness = Stiffness;

	SuspensionSettings.DampingCoefficient = Damping;

	SuspensionSettings.SuspensionLength = SuspensionLength;
}

UTire::~UTire()
{
}

float UTire::GetTraction(const float ThrottleForce) const
{
	return IsFrontTire && IsGrounded ? FMath::Clamp(ThrottleForce, -MaximumWheelTraction, MaximumWheelTraction) : 0;
}

float UTire::GetWheelBrakingForce(const float BrakingForce) const
{
	//Calculate the Tire ratio
	float TireLoadRatio = TireLoad / (BaseTireLoad>0?BaseTireLoad:TireLoad);
	//Apply the ratio to the braking force
	return TireLoadRatio*BrakingForce;
}
