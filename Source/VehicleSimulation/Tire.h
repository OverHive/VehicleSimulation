// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "WheelSuspensionSetting.h"
#include "Tire.generated.h"
enum TIREINDEX { REARLEFT = 0, REARRIGHT = 1, FRONTLEFT = 2, FRONTRIGHT = 3 };
/**
 *
 */

static float Gravity = 981.0f;
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VEHICLESIMULATION_API UTire : public UStaticMeshComponent
{
	GENERATED_BODY()
public:
	// Sets default values for this actor's properties
	UTire();
	~UTire();
	//Calculates the traction of the tire
	float GetTraction(const float ThrottleForce) const;
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
	//Gets the current suspension force
	float GetSuspensionForce() { return IsGrounded ? SuspensionForce : 0; }
	//Get the wheel's tire load
	float GetTireLoad() const { return TireLoad; };
	//Updates the rotational velocity of the wheel
	void UpdateWheelRotationalVelocity(const float VehicleSpeed);
	//Gets the friction coefficient 
	float GetFrictionCoefficient() const { return FrictionCoefficient; }
	//Model the force using slip
	float MagicFormula(const float value, const float x) const;
	//Returns the tire's slip angle
	float GetSlipAngle() const { return SlipAngle; };
	//Gets the lateral grip of the tire
	const float GetLateralGrip();
	//Update the frictionCoefficient
	void UpdateFrictionCoefficient(const float value) { FrictionCoefficient = value; }
	//Change the radius of the tire
	void UpdateTireRadius(const float value) { SuspensionSettings.WheelRadius = value; }
	//Calculates the maximum load the tire can bear
	void UpdateMaxTraction();

	//Updates the steering direction of the tire
	void UpdateSteering(const float NewAngle);
	//Store the contact location of the tire
	void StoreTireContactInformation(const  FHitResult  NewHitLocation);
	//Update the suspension setting of the wheels
	void UpdateSuspension(const float Stiffness, const float Damping, const float SuspensionLength, const float TireStiffness, const float UnSprungMass, const float StaticTireLoad);
	//Updates the tire's slip ratio
	void UpdateSlipRatio(const float wheelSpeed, const bool IsBraking);
	//Updates the current tire load
	void UpdateTireLoad(float NormalForce);
	//Updates the tire's angle ratio
	void UpdateSlipAngle(const float velocityY, const float velocityX);

	//Gets the wheel's contact point
	FVector GetContactPoint() const { return ContactPoint; }
	//Returns the direction of the normal force on the wheel 
	FVector GetContactNormal() const { return ContacNormal; }
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	float SteerAngle = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	bool IsFrontTire = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	bool IsRightTire = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tire")
	//It dictates how quickly the tire builds up grip as slip 
	float StiffnessFactor = 1.5f;
	//Determines the overall shape of the curve 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tire")
	float ShapeFactor = 1.3f;
	//Determines how much grip is lost once the tire starts sliding 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tire")
	float CurvatureFactor = 0.5f;

	//For calculating rolling resistance
	float RollingResistanceCoefficient = 0.015f;
	FName SocketName;
	TIREINDEX TirePosition;
	FWheelSuspensionSetting SuspensionSettings = FWheelSuspensionSetting();

	float TireLoad = 0;
	bool IsGrounded = false;
private:

	FVector ContactPoint = FVector::ZeroVector;
	FVector ContacNormal = FVector::ZeroVector;
	float FrictionCoefficient = 1.4;
	float SlipRatio = 0.0;
	float SlipAngle = 0.0;

	float MaximumWheelTraction = 0.5;
	float WheelDamper = 0.98f;
	float CouplingFactor = 0.1f;
	float WheelRotationalVelocity = 0.0f;
	float WheelRotationalInertia = 0.5f;
	float LastBrakeTorque = 0.0f;
	float SuspensionForce = 0;

	float TireCompression = 0.0f;
	float SuspensionCompression = 0.0f;
};
