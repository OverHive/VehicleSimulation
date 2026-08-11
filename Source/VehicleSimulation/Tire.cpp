// Fill out your copyright notice in the Description page of Project Settings.


#include "Tire.h"


UTire::UTire()
{
}

void UTire::UpdateMaxGrip()
{
	MaxGrip = FrictionCoefficient * TireLoad;
}

const float UTire::GetLateralGrip()
{
	return IsGrounded ? -MagicFormula(MaxGrip, GetSlipAngle()) : 0;
}

float UTire::FrictionCircle(const float LongitudinalForce, const float LateralForce, const bool LongitudinalLeading) const
{
	//Get maximum force for selected force through a rearrange friction circle
	float MaxForce =FMath::Pow(MaxGrip, 2) - FMath::Pow(LongitudinalLeading ?LongitudinalForce:LateralForce, 2);
	//Return the result
	return MaxForce > 0 ? FMath::Sqrt(MaxForce) : 0;

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
	UpdateMaxGrip();
}

void UTire::UpdateSlipRatio(const float VehicleSpeedAtWheel, const bool IsBraking)
{
	float WheelSurfaceSpeed = GetRotationalVelocity();
	float Denominator = FMath::Abs(IsBraking ? VehicleSpeedAtWheel : WheelSurfaceSpeed);
	SlipRatio = FMath::Abs(Denominator) > 0.1f ? (WheelSurfaceSpeed - VehicleSpeedAtWheel) / Denominator : 0;
}

void UTire::UpdateSlipAngle(const float LongitudinalVelocity, const float LateralVelocity)
{
	SlipAngle = FMath::Atan2(LateralVelocity, LongitudinalVelocity) - SteerAngle;
}
void UTire::UpdateRollingRadius(FVector AxisPosition)
{
	RollingRadius = !ContactPoint.IsNearlyZero()
		? FVector::Dist2D(ContactPoint, AxisPosition)
		: 0;
}
float UTire::GetRollingResistance() const
{
	return IsGrounded ? TireLoad * RollingResistanceCoefficient : 0;
}

float UTire::GetCompression(const float CurrentDistance)
{
	//Calculate the spring compression using the difference between the suspension 
	//length and the bottom of the wheel
	SuspensionCompression = FMath::Max(0.0f, SuspensionSettings.RestPosition - CurrentDistance);
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

	// As the compression of the tire makes it act like a spring we can get the force 
	// it provides with to the suspension by multiplying compression by tire stiffness.
	float TireSpringForce = TireCompression * SuspensionSettings.TireVerticalStiffness;

	// Total Force = Spring + Tire  - Damping (Damping opposes the velocity)
	SuspensionForce = SpringForce + TireSpringForce - DampingForce;
	//Clamp the total force to prevent negative values 
	SuspensionForce = FMath::Max(0.0f, SuspensionForce);
	return IsGrounded ? SuspensionForce : 0;
}

void UTire::UpdateWheelRotationalVelocity(const float NetTorque, const float DeltaTime)
{
	//If the tire is in the air enter free rotation
	if (!IsGrounded)
	{
		WheelRotationalVelocity *= WheelDamper;
		return;
	}
	float RotationalAcceleration = Inertia != 0 ? NetTorque / Inertia : 0;
	//Update the rotation velocity with the acceleration
	WheelRotationalVelocity += WheelRotationalInertia * DeltaTime;
}

float UTire::MagicFormula(const float peakValue, const float x) const
{
	float StiffnessEffect = StiffnessFactor * x;
	float CurvatureEffect = CurvatureFactor * (StiffnessEffect - FMath::Atan(StiffnessEffect));
	float arc = FMath::Atan(StiffnessEffect - CurvatureEffect);
	return peakValue * sin(ShapeFactor * arc);
}

void UTire::StoreTireContactInformation(const FHitResult  Hit)
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

	//Get the static force on the string with is equal to the unsprung weight subtract from the tire load 
	float ForceOnSpring = StaticTireLoad;

	//Calculate the resting suspension length

	SuspensionSettings.RestPosition = ForceOnSpring / SuspensionSettings.SpringStiffness;
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
