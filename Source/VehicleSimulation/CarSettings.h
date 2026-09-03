// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarSettings.generated.h"

/** The class for creating individual vehicle presents
 *
 */
USTRUCT(BlueprintType)
struct VEHICLESIMULATION_API FCarSettings
{
	GENERATED_BODY()
	FCarSettings() {};
	//|---------------------------------Vehicle class -----------------------------|
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VehicleName")
	FName VehicleName = "preset";
	float TotalVehicleMass = 10.f;
	//|---------------------------------Vehicle dimensions and mass ------------------------|
	float VehicleSprungMass = 0.0f;
	float Height = 0.0f;
	float Width = 0.0f;
	float DragCoefficient = 0.0f;
	float WheelBaseLength = 0.0f;
	float CentreOfGravityHeight = 0.0f;
	float DistanceOfCentreOfGravityToFrontAxis = 0.0f;
	float DistanceOfCentreOfGravityToRearAxis = 0.0f;
	float TrackWidth = 0.0f;
	//|---------------------------------Pitch parameters --------------------------|
	float PitchInertia = 0.0f;
	float PitchStiffness = 0.0f;
	float PitchDamping = 0.0f;
	//|---------------------------------Heave parameters --------------------------|
	float HeaveDamping = 0.0f;
	//|---------------------------------Front axis --------------------------------|
	float FrontSuspensionStiffness = 0.0f;
	float FrontSuspensionDamping = 0.0f;
	float FrontUnSprungDamping = 0.0f;
	float FrontUnsprungMass = 0.0f;
	float FrontTireVerticalStiffness = 0.0f;
	//|---------------------------------Front wheels ------------------------------|
	float FrontWheelRadius = 0.0f;
	float FrontTireFriction = 0.0f;
	float FrontWheelInertia = 0.0f;
	float FrontRollingResistanceCoefficient = 0.0f;
	//|---------------------------------Rear axis ---------------------------------|
	float RearSuspensionStiffness = 0.0f;
	float RearSuspensionDamping = 0.0f;
	float RearUnsprungMass = 0.0f;
	float RearTireVerticalStiffness = 0.0f;
	float RearUnSprungDamping = 0.0f;
	float FrontBrakeTorque = 0.0f;
	//|---------------------------------Rear wheels -------------------------------|
	float RearWheelRadius = 0.0f;
	float RearTireFriction = 0.0f;
	float RearWheelInertia = 0.0f;
	float RearRollingResistanceCoefficient = 0.0f;
	float RearBrakeTorque = 0.0f;
	//|---------------------------------Engine parameters -------------------------|
	TArray<float> GearRatios;
	float FinalDriveRatio = 0.0f;
	float DrivetrainEfficiency = 0.0f;
	//The small RPM in the curve that produces a value
	float MinimumStartingRPM = 0;
	float CurveStep = 0;
	TMap <float, float> TorqueCurve;
	float MaxRPM = 0;
	//|--------------------------------- Tire features ---------------------------------------|
	TArray<float> StiffnessFactors = { 0.0f,0.0f,0.0f };
	TArray<float> ShapeFactors = {0.0f,0.0f,0.0f};
	TArray<float> CurvatureFactors = {0.0f,0.0f,0.0f};
	//|-------------------------------- End ---------------------------------------|
	FCarSettings(const FName NewVehicleName, const float NewVehicleSprungMass, const float NewHeight, const float NewWidth,
		const float NewDragCoefficient, const float NewWheelBaseLength, const float NewCOFHeight, const float NewFrontAxisToCOF,
		const float NewRearAxisToCOF, const float NewTrackWidth, const TArray<float> NewGearRatios, const float NewFinalDriveRatio,
		const float NewDrivetrainEfficiency, const float NewPitchInertia, const float NewPitchStiffness, const float NewPitchDamping,
		const float NewFrontSuspensionStiffness, const float NewRearSuspensionStiffness, const float NewFrontSuspensionDamping, const float NewRearSuspensionDamping,
		const float NewFrontUnSprungDamping, const float NewRearUnSprungDamping, const float NewFrontUnsprungMass, const float NewRearUnsprungMass,
		const float NewFrontTireVerticalStiffness, const float NewRearTireVerticalStiffness, const float NewFrontWheelRadius, const float NewRearWheelRadius,
		const float NewFrontTireFriction, const float NewRearTireFriction, const float NewHeaveDamping, TMap <float, float> NewTorqueCurve, float const NewCurveStep
		, const float NewMaximumRPM, const float NewMinimumStartingRPM, const float NewFrontRollingResistanceCoefficient, const float NewRearRollingResistanceCoefficient,
		const float NewFrontWheelInertia, const float NewRearWheelInertia, const float NewFrontBrakeTorque, const float NewRearBrakeTorque,
		const TArray<float> NewStiffnessFactors, const TArray<float> NewShapeFactors, const TArray<float> NewCurvatureFactors)
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
		PitchDamping = NewPitchDamping;
		HeaveDamping = NewHeaveDamping;

		FrontSuspensionStiffness = NewFrontSuspensionStiffness;
		RearSuspensionStiffness = NewRearSuspensionStiffness;
		FrontTireVerticalStiffness = NewFrontTireVerticalStiffness;
		RearTireVerticalStiffness = NewRearTireVerticalStiffness;

		CurvatureFactors = NewCurvatureFactors;
		StiffnessFactors = NewStiffnessFactors;
		ShapeFactors = NewShapeFactors;

		//Convert from m^2 to cm^2
		PitchInertia = NewPitchInertia * 10000;
		FrontWheelInertia = NewFrontWheelInertia * 10000;
		RearWheelInertia = NewRearWheelInertia * 10000;
		FrontBrakeTorque = NewFrontBrakeTorque * 10000;
		RearBrakeTorque = NewRearBrakeTorque * 10000;
		//Convert from m to cm
		FrontWheelRadius = NewFrontWheelRadius * 100;
		RearWheelRadius = NewRearWheelRadius * 100;
		Height = NewHeight * 100;
		Width = NewWidth * 100;
		WheelBaseLength = NewWheelBaseLength * 100;
		CentreOfGravityHeight = NewCOFHeight * 100;
		DistanceOfCentreOfGravityToFrontAxis = NewFrontAxisToCOF * 100;
		DistanceOfCentreOfGravityToRearAxis = NewRearAxisToCOF * 100;
		TrackWidth = NewTrackWidth * 100;

		TotalVehicleMass = VehicleSprungMass + 2 * (FrontUnsprungMass + RearUnsprungMass);
	}
};
