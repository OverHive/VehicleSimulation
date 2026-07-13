// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "WheelSuspensionSetting.h"
#include "Tire.generated.h"

/**
 * 
 */

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
	//Calculates the braking for the tire
	float GetWheelBrakingForce(const float brakingForce);
	//Update the frictionCoefficient
	void UpdateFrictionCoefficient(const float value) { FrictionCoefficient = value; }
	//Change the radius of the tire
	void UpdateTireRadius(const float value) { TireRadius = value; }
	//Calculates the maximum load the tire can bear
	void UpdateMaxTraction();
	//Gets the lateral grip of the tire
	const float GetLateralGrip();
	//Updates the steering direction of the tire
	void UpdateSteering(const float NewAngle);
	//Update the vehicle fields the wheel has	
	void UpdateVehicleParameters(const float mass, const float wheelBaseLength,const float trackWidth, const float DistanceOfCGToFrontAxis, const float DistanceOfCGToRearAxis, const float CGHeight);
	//Update the suspension setting of the wheels
	void UpdateSuspension(const float stiffness, const float damping, const float SuspensionLength = 50.0);
	//Updates the wheel's suspension
	void UpdateWheelSuspension(const FVector NewSpringForce, const  FVector NewHitLocation);
	//Calculate the current tire load
	void UpdateTireLoad(const float LongitudinalAcceleration, const float LateralAcceleration);
	//Updates the rotational velocity of the wheel
	void UpdateWheelRotationalVelocity(const float VehicleSpeed);
	//Get the angle velocity of the wheel
	float GetRotationalVelocity() { return WheelRotationalVelocity * (WheelRadius / 100.0f); };
	//Apply deceleration to the wheel
	void ApplyBrakes(const float BrakeTorque, const float DeltaTime);
	//Calculate the rolling resistance
	float GetRollingResistance() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	float SteerAngle = 0.0f;   // degrees, yaw relative to the chassis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	bool IsFrontTire = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	bool IsRightTire = false;
	//For calculating rolling resistance
	float RollingResistanceCoefficient = 0.015f; 
	FName SocketName;
	FWheelSuspensionSetting SuspensionSettings = FWheelSuspensionSetting();
	FVector ContactPoint = FVector::ZeroVector;	
	float TireLoad = 0;
	bool IsGrounded = false;
private:
 
	float Gravity = 981;
	float VehicleMass = 300;
	float BaseTireLoad = 0;
	float VehicleWeight = 300000;
	float FrictionCoefficient = 1.4;
	float TireRadius = 1;
	float TrackWidth = 0;

	float MaximumWheelTraction = 0.5;
	float DistanceOfCentreOfGravityToTireAxis = 0.5;
	float WheelBase = 1;
	float CentreOfGravityHeight = 1;
	float WheelDamper = 0.98f;
	float CouplingFactor = 0.1f;
	float WheelRotationalVelocity = 0.0f;   
	float WheelRotationalInertia = 0.5f;     
	float WheelRadius = 30.0f;               
	float LastBrakeTorque = 0.0f;          

	
	FVector NormalForce = FVector::ZeroVector;
};
