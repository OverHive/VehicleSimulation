// Fill out your copyright notice in the Description page of Project Settings.


#include "CarSettings.h"

CarSettings::CarSettings()
{
}

CarSettings::CarSettings(const FName NewVehicleName, const float NewVehicleSprungMass, const float NewHeight, const float NewWidth,
	const float NewDragCoefficient, const float NewWheelBaseLength, const float NewCOFHeight, const float NewFrontAxisToCOF,
	const float NewRearAxisToCOF, const float NewTrackWidth, const TArray<float> NewGearRatios, const float NewFinalDriveRatio,
	const float NewDrivetrainEfficiency, const float NewPitchInertia, const float NewPitchStiffness, const float NewPitchDamping,
	const float NewFrontSuspensionStiffness, const float NewRearSuspensionStiffness, const float NewFrontSuspensionDamping, const float NewRearSuspensionDamping,
	const float NewFrontUnSprungDamping, const float NewRearUnSprungDamping, const float NewFrontUnsprungMass, const float NewRearUnsprungMass,
	const float NewFrontTireVerticalStiffness, const float NewRearTireVerticalStiffness, const float NewFrontWheelRadius, const float NewRearWheelRadius,
	const float NewFrontTireFriction, const float NewRearTireFriction, const float NewHeaveDamping, TMap <float, float> NewTorqueCurve, float const NewCurveStep
	, const float NewMaximumRPM, const float NewMinimumStartingRPM, const float NewFrontRollingResistanceCoefficient, const float NewRearRollingResistanceCoefficient,
	const float NewFrontWheelInertia, const float NewRearWheelInertia)
{
	MinimumStartingRPM = NewMinimumStartingRPM;
	VehicleName = NewVehicleName;
	VehicleSprungMass = NewVehicleSprungMass;
	DragCoefficient = NewDragCoefficient;
	GearRatios = NewGearRatios;
	FinalDriveRatio = NewFinalDriveRatio;
	DrivetrainEfficiency = NewDrivetrainEfficiency;
	FrontUnsprungMass = NewFrontUnsprungMass;
	RearUnsprungMass = NewRearUnsprungMass;
	FrontTireFriction = NewFrontTireFriction;
	RearTireFriction = NewRearTireFriction;
	TorqueCurve = NewTorqueCurve;
	CurveStep = NewCurveStep;
	MaxRPM = NewMaximumRPM;
	FrontRollingResistanceCoefficient = NewFrontRollingResistanceCoefficient;
	RearRollingResistanceCoefficient = NewRearRollingResistanceCoefficient;
	PitchStiffness = NewPitchStiffness;
	FrontSuspensionDamping = NewFrontSuspensionDamping;
	RearSuspensionDamping = NewRearSuspensionDamping;
	FrontUnSprungDamping = NewFrontUnSprungDamping;
	RearUnSprungDamping = NewRearUnSprungDamping;
	HeaveDamping = NewHeaveDamping;

	FrontSuspensionStiffness = NewFrontSuspensionStiffness;
	RearSuspensionStiffness = NewRearSuspensionStiffness;
	FrontTireVerticalStiffness = NewFrontTireVerticalStiffness;
	RearTireVerticalStiffness = NewRearTireVerticalStiffness;

	//Convert from m^2 to cm^2
	PitchInertia = NewPitchInertia * 1000;
	FrontWheelInertia = NewFrontWheelInertia * 1000;
	RearWheelInertia = NewRearWheelInertia * 1000;
	//Convert from m to cm
	FrontWheelRadius = NewFrontWheelRadius;
	RearWheelRadius = NewRearWheelRadius;
	Height = NewHeight * 100;
	Width = NewWidth * 100;
	WheelBaseLength = NewWheelBaseLength * 100;
	CentreOfGravityHeight = NewCOFHeight * 100;
	DistanceOfCentreOfGravityToFrontAxis = NewFrontAxisToCOF * 100;
	DistanceOfCentreOfGravityToRearAxis = NewRearAxisToCOF * 100;
	TrackWidth = NewTrackWidth * 100;
	PitchDamping = NewPitchDamping * 100;
}

CarSettings::~CarSettings()
{
}
