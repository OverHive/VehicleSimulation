// Fill out your copyright notice in the Description page of Project Settings.


#include "Tire.h"

UTire::UTire()
{
}

void UTire::UpdateMaxTireLoad()
{
	MaxTireLoad = FrictionCoefficient * SpringForce.Z;
}

float UTire::UpdateTireLoad(float Acceleration)
{
	//Calculate the change in tire load based on the force acting on the vehicle by 
	//multiplying  the ratio between the height of the vehicle's centre of gravity 
	//and it's wheelbase multiplied by the force acting on the vehicle
	//(calculated using force = mass * acceleration)
	float ForceOnVehicle = Acceleration * VehicleMass;
	float ChangeInTireLoad = WheelBase>0? (CentreOfGravityHeight / WheelBase) * ForceOnVehicle:0;
	//Calculate the default weight acting on the tire by multiply the ratio between the tire's axis
	//to the vehicle's centre of gravity and the wheelbase by the
	//vehicle's weight (calculated using force = mass * acceleration due to gravity)
	float DefaultTireLoad = WheelBase > 0 ? (DistanceOfCentreOfGravityToTireAxis / WheelBase) * (VehicleMass * Gravity):0;
	//Apply the change in load based on the position of the tire 
	TireLoad =  DefaultTireLoad + (IsFrontTire?-1:1)*ChangeInTireLoad;
	//Prevent negative tire load
	if (TireLoad < 0)
	{
		TireLoad = 0;
	}
	return TireLoad/2;
}

void UTire::UpdateVehicleParameters(const float mass, const float wheelBaseLength, const float DistanceOfCGToTireAxis, const float CGHeight)
{
	VehicleMass = mass;
	WheelBase = wheelBaseLength;
	DistanceOfCentreOfGravityToTireAxis = DistanceOfCGToTireAxis;
	CentreOfGravityHeight = CGHeight;

}

void UTire::UpdateWheelSuspension(const FVector NewSpringForce, const FVector NewHitLocation)
{
	SpringForce = NewSpringForce;
	HitLocation = NewHitLocation;
}

UTire::~UTire()
{
}
