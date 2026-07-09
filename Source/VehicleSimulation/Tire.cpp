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

void UTire::UpdateTireLoad(const float LongitudinalAcceleration, const float LateralAcceleration)
{
	//Calculate the change in tire load based on tire positions

	//Calculate the longitudinal change in tire load 
	float LongitudinalForceOnVehicle = LongitudinalAcceleration * VehicleMass;
	float LongitudinalChangeInTireLoad = WheelBase > 0 ? (CentreOfGravityHeight / WheelBase) * (IsFrontTire ? -1 : 1) * LongitudinalForceOnVehicle : 0;

	//Calculate the lateral change in tire load 
	float LateralForceOnVehicle = LateralAcceleration * VehicleMass;
	float LateralChangeInTireLoad = TrackWidth > 0 ? (IsRightTire ? 1 : -1) * LateralForceOnVehicle * CentreOfGravityHeight / TrackWidth : 0;
	//Calculate the default weight acting on the tire
	float DefaultTireLoad = NormalForce.Size();//WheelBase > 0 ? (DistanceOfCentreOfGravityToTireAxis / WheelBase)*BaseTireLoad : 0;  : 0;
	//Apply the change in load 
	TireLoad = FMath::Max(0.0f, DefaultTireLoad + LongitudinalChangeInTireLoad + LateralChangeInTireLoad);
	//Prevent negative tire load and tire load for aerial tires
	if (TireLoad < 0 || !IsGrounded)
	{
		TireLoad = 0;
	}
	//Update the maximum traction force on the tire with the new tire load
	UpdateMaxTraction();
}

float UTire::GetRollingResistance()
{
	return TireLoad * RollingResistanceCoefficient;
}

void UTire::UpdateVehicleParameters(const float mass, const float wheelBaseLength, const float trackWidth, const float DistanceOfCGToFrontAxis, const float DistanceOfCGToRearAxis, const float CGHeight)
{
	VehicleMass = mass;

	VehicleWeight = mass * Gravity;
	BaseTireLoad = VehicleWeight / 4;
	WheelBase = wheelBaseLength;
	TrackWidth = trackWidth;
	DistanceOfCentreOfGravityToTireAxis = IsFrontTire ? DistanceOfCGToFrontAxis : DistanceOfCGToRearAxis;
	CentreOfGravityHeight = CGHeight;

}

void UTire::UpdateSuspension(const float stiffness, const float damping, const float SuspensionLength)
{
	SuspensionSettings.SpringStiffness = stiffness;

	SuspensionSettings.DampingCoefficient = damping;

	SuspensionSettings.SuspensionLength = SuspensionLength;
}

void UTire::UpdateWheelSuspension(const FVector NewSpringForce, const FVector NewHitLocation)
{
	const float LoadFactor = TireLoad / VehicleWeight;
	NormalForce = NewSpringForce;
	ContactPoint = NewHitLocation;
}

UTire::~UTire()
{
}

float UTire::GetTraction(const float ThrottleForce) const
{
	return IsFrontTire && IsGrounded ? FMath::Clamp(ThrottleForce / 2.f, -MaximumWheelTraction, MaximumWheelTraction) : 0;
}
