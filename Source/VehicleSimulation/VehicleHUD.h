// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tire.h"
#include <functional>

/**
 *
 */
class VEHICLESIMULATION_API VehicleHUD
{
public:
	VehicleHUD();
	//Displays data related to the vehicle body
	void BodyHUD(float ForwardVelocity, float LongitudinalAcceleration,float CurrentThrottle, float CurrentBrake,
		float CurrentSteeringAngle,float CurrentSteering, int GearIndex, float CurrentDrag,
		float GearRatio, float FrontAxleShare, float RearAxleShare, float VehicleMass, float VerticalVelocity, float VehicleWeight,
		int VehiclePresetIndex,FName VehiclePresetName,float CurrentDrivingForce,float EngineCapType);
	//Displays data related to the wheels
	void WheelHUD(TArray<UTire*> Tires, TFunction <float(UTire* Tire)> TractionFunction);
	//Displays data related to the suspension
	void SuspensionHUD(TArray<UTire*> Tires, float PitchAngle, float HeavePosition);
	~VehicleHUD();
};
