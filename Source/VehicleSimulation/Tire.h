// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


/**
 * 
 */
class VEHICLESIMULATION_API Tire
{
public:
	Tire();
	~Tire();
	//Update the frictionCoefficient
	void UpdateFrictionCoefficient(const float value) { FrictionCoefficient = value; }
	//Change the radius of the tire
	void UpdateTireRadius(const float value) { TireRadius = value; }
	//Calculates the maximum load the tire can bear
	void UpdateMaxTireLoad();
	//Calculate the current tire load
	void UpdateTireLoad();
	//Update the vehicle fields the wheel has
	void UpdateVehicleParameters(const float mass, const float wheelBaseLength, const float DistanceOfCGToTireAxis, const float CGHeight);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	bool isFrontTire = true;
private:
 
	float gravity = 10;
	float VehicleMass = 300;
	float FrictionCoefficient = 0.5;
	float TireRadius = 1;
	float TireLoad = 0;
	float Acceleration = 0.5;
	float MaxTireLoad = 0.5;
	float DistanceOfCentreOfGravityToTireAxis = 0.5;
	float WheelBase = 1;
	float CentreOfGravityHeight = 1;
};
