// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "WheelSuspensionSetting.h"
#include "WheelConfiguration.h"
#include "Tire.generated.h"

//For selecting the appropriate factors for Magic formula based on the action of the wheel
static enum  WHEELMODE { ACCELERATION = 0, BRAKING = 1, CORNERING = 2 };
static float Gravity = 980.0f;
static FVector GetMeshDimensions(UStaticMeshComponent* MeshComponent)
{
	FVector Min, Max;
	//Get the  bounds of the mesh
	MeshComponent->GetLocalBounds(Min, Max);
	//Multiply the bounds to get the size
	FVector MeshSize = Max - Min;
	return MeshSize;
}

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VEHICLESIMULATION_API UTire : public UStaticMeshComponent
{
	GENERATED_BODY()
public:
	// Sets default values for this actor's properties
	UTire();
	~UTire();
	//Clamps a force to the tire's  max grip
	float ClampToTireGrip(const float ThrottleForce) const;
	//Returns the angle velocity of the wheel
	float GetRotationalVelocity() { return WheelRotationalVelocity; };
	//Returns the float slip ratio
	float GetSlipRatio() const { return SlipRatio; };
	//Calculate the rolling resistance
	float GetRollingResistance() const;
	//Get the compression of the wheel
	float GetCompression(const float CurrentDistance);
	//Gets the overall suspension force on the tire
	float CalculateSuspensionForce(const float SuspensionVelocity);
	//Gets the normal force on the tire
	float GetNormalForce() const;
	float CalculateRotationalAcceleration(const float CurrentWheelRotationalVelocity, const float LongitudinalForceMagnitude, const float ResistiveForce, const float torque_multiplier, const float GroundSpeed, const float DeltaTime) const;
	//Gets the current suspension force
	float GetSuspensionForce() { return IsGrounded ? SuspensionForce : 0; }
	//Get the wheel's tire load
	float GetTireLoad() const { return TireLoad; };
	//Gets the friction coefficient 
	float GetFrictionCoefficient() const { return FrictionCoefficient; }
	//Model the force using slip
	float MagicFormula(const float value, const float x, const int Index) const;
	//Returns the tire's slip angle
	float GetSlipAngle() const { return SlipAngle; };
	//Returns the rolling radius of wheel
	float GetRollingRadius() const { return RollingRadius; };
	//Return the braking force of the tire
	float GetBrakingForce() const;
	//Applies the friction circle to a given force
	float FrictionCircle(const float LongitudinalForce, const float LaterialForce, const bool LongitudinalLeading) const;
	//Get the inertia of the wheels
	float GetWheelInertia() const { return Inertia; };
	//Calculates the peak slips values
	float CalculatePeakSlip(const int Index) const;
	//Returns peak slip values
	float GetPeakSlips(const int Index);
	//Returns the lateral force on the tire
	FVector GetLateralForceVector() const;
	//Update the frictional coefficient of the tire
	void UpdateTireFrictionCoefficient(const float NewValue);
	//Update the friction coefficient
	void UpdateFrictionCoefficient(const float NewValue);
	//Update the rolling resistance coefficient
	void UpdateRollingResistanceCoefficient(const float NewValue) { RollingResistanceCoefficient = NewValue; }
	//Change the radius of the tire
	void UpdateTireRadius(const float value);
	//Calculates the maximum load the tire can bear
	void UpdateMaxGrip();
	//Updates the rotational velocity of the wheel
	void UpdateWheelRotationalVelocity(const float DriveForce, const float MaxBrakingForce, const float BrakeStrength, const float GroundSpeed, const float DeltaTime);
	//Updates the steering direction of the tire
	void UpdateSteering(const float NewAngle);
	//Store the contact location of the tire
	void StoreTireContactInformation(const  FHitResult  NewHitLocation);
	//Update the suspension setting of the wheels
	void UpdateSuspension(const float Stiffness, const float Damping, const float SuspensionLength, const float TireStiffness, const float UnSprungMass, const float StaticTireLoad);
	//Updates the tire's slip ratio
	void UpdateSlipRatio(const float CurrentRotationalVelocity, const float VehicleSpeed, const bool IsBraking);
	//Updates the current tire load
	void UpdateTireLoad(float NormalForce);
	//Updates the tire's angle ratio
	void UpdateSlipAngle(const float velocityY, const float velocityX);
	//Calculates the rolling radius of the wheel
	void UpdateRollingRadius(FVector AxisPosition);
	//Update the wheel's Inertia
	void UpdateWheelInertia(float NewInertia) { Inertia = NewInertia; };
	//Updates the braking torque of the wheel
	void UpdateBrakingTorque(const float NewBrakingTorque) { BrakingTorque = NewBrakingTorque; }
	//Updates the Magic formula parameters
	void UpdateWheelFeatures(const TArray<float> NewStiffnessFactors, const TArray<float> NewShapeFactors, const TArray<float> NewCurvatureFactors);
	//Updates the wheels configuration
	void UpdateWheelConfiguration(const FWheelConfiguration NewConfig) { WheelConfig = NewConfig; };
	// Handles resizing the wheel
	void UpdateWheelWorldPosition(FVector const MeshScale);
	//Handles updating the Rotational velocity when under Magic formula threshold
	void ClampToVehicleWheelSpeed(const float WheelSpeed, const float DeltaTime, const bool IsBraking);
	//Gets the starting dimensions of the tire mesh
	void StoreInitialTireMeshDimensions();
	//Gets the wheel's contact point
	FVector GetContactPoint() const { return ContactPoint; }
	//Returns the direction of the normal force on the wheel 
	FVector GetContactNormal() const { return ContacNormal; }
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class  UStaticMeshComponent* WheelMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	FWheelConfiguration WheelConfig;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	float SteerAngle = 0.0f;
	float Inertia = 8500; //Kg/ cm^2
	//It dictates how quickly the tire builds up grip as slip 
	TArray<float> StiffnessFactors;
	//Determines the overall shape of the curve 
	TArray<float> ShapeFactors;
	//Determines how much grip is lost once the tire starts sliding 
	TArray<float> CurvatureFactors;

	//For calculating rolling resistance
	float RollingResistanceCoefficient = 0.015f;
	FName SocketName;
	FWheelSuspensionSetting SuspensionSettings = FWheelSuspensionSetting();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	TArray<float> PeakSlips;
	float TireLoad = 0;
	bool IsGrounded = false;
private:

	//Updates the total grip of the tire
	void UpdateTotalGrip() { TireGrip = FrictionCoefficient + TireFrictionCoefficient; };
	FVector ContactPoint = FVector::ZeroVector;
	FVector ContacNormal = FVector::ZeroVector;
	float RollingRadius = 0.0f;
	float FrictionCoefficient = 1.4f;
	float TireFrictionCoefficient = 1.4f;
	float TireGrip = 1.4f;
	float SlipRatio = 0.0;
	float SlipAngle = 0.0;


	float MaxGrip = 0.5;
	float WheelDamper = 0.98f;
	float CouplingFactor = 0.1f;
	float WheelRotationalVelocity = 0.0f;
	float SuspensionForce = 0;
	float NormalForce = 0.0f;
	float BrakingTorque = 0;
	float TireCompression = 0.0f;
	float SuspensionCompression = 0.0f;
	FVector WheelMeshDimension = FVector::ZeroVector;
	FVector WheelPosition = FVector::ZeroVector;
};
