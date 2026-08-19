// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/** The class for creating individual vehicle presents
 *
 */
class VEHICLESIMULATION_API CarSettings
{
public:
	float VehicleSprungMass = 0.0f;
	float Height = 0.0f;
	float Width = 0.0f;
	float DragCoefficient = 0.0f;
	float WheelBaseLength = 0.0f;
	float CentreOfGravityHeight = 0.0f;
	float DistanceOfCentreOfGravityToFrontAxis = 0.0f;
	float DistanceOfCentreOfGravityToRearAxis = 0.0f;
	float TrackWidth = 0.0f;
	TArray<float> GearRatios;
	float FinalDriveRatio = 0.0f;
	float DrivetrainEfficiency = 0.0f;
	float PitchInertia = 0.0f;
	float PitchStiffness = 0.0f;
	float PitchDamping = 0.0f;
	float FrontSuspensionStiffness = 0.0f;
	float RearSuspensionStiffness = 0.0f;
	float FrontSuspensionDamping = 0.0f;
	float RearSuspensionDamping = 0.0f;
	float FrontUnSprungDamping = 0.0f;
	float RearUnSprungDamping = 0.0f;
	float FrontUnsprungMass = 0.0f;
	float RearUnsprungMass = 0.0f;
	float FrontTireVerticalStiffness = 0.0f;
	float RearTireVerticalStiffness = 0.0f;
	float FrontWheelRadius = 0.0f;
	float RearWheelRadius = 0.0f;
	float FrontTireFriction = 0.0f;
	float RearTireFriction = 0.0f;
	float HeaveDamping = 0.0f;
	float MaxTorque = 0;
	float CurveStep = 0;
	TMap <float, float> TorqueCurve;
	FName VehicleName = "preset";
	CarSettings();
	CarSettings(const FName NewVehicleName, const float NewVehicleSprungMass,  const float NewHeight, const float NewWidth,
		const float NewDragCoefficient, const float NewWheelBaseLength, const float NewCOFHeight, const float NewFrontAxisToCOF,
		const float NewRearAxisToCOF, const float NewTrackWidth, const TArray<float> NewGearRatios, const float NewFinalDriveRatio,
		const float NewDrivetrainEfficiency,const float NewPitchInertia, const float NewPitchStiffness, const float NewPitchDamping,
		const float NewFrontSuspensionStiffness, const float NewRearSuspensionStiffness, const float NewFrontSuspensionDamping, const float NewRearSuspensionDamping,
		const float NewFrontUnSprungDamping, const float NewRearUnSprungDamping,const float NewFrontUnsprungMass, const float NewRearUnsprungMass,
		const float NewFrontTireVerticalStiffness, const float NewRearTireVerticalStiffness, const float NewFrontWheelRadius, const float NewRearWheelRadius,
		const float NewFrontTireFriction, const float NewRearTireFriction, const float NewHeaveDamping, TMap <float, float> NewTorqueCurve, float const NewCurveStep);
	~CarSettings();
};
