// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "WheelSuspensionSetting.h"
#include "Tire.generated.h"
enum TIREINDEX{ REARLEFT = 0, REARRIGHT = 1, FRONTLEFT = 2, FRONTRIGHT = 3 };
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
	float GetWheelBrakingForce(const float brakingForce) const;
	//Returns the angle velocity of the wheel
	float GetRotationalVelocity() { return WheelRotationalVelocity * (SuspensionSettings.WheelRadius ); };
	//Returns the float slip ratio
	float GetSlipRatios() const { return SlipRatio; };
	//Calculate the rolling resistance
	float GetRollingResistance() const;
	//Get the compression of the wheel
	float GetCompression(const float CurrentDistance);
	//Gets the overall suspension force on the tire
	float CalculateSuspensionForce(const float SuspensionVelocity);
	//Gets the current suspension force
	float GetSuspensionForce() { return SuspensionForce; }
	
	//Model the force using slip
	float MagicFormula(const float value, const float x) const;
	//Gets the lateral grip of the tire
	const float GetLateralGrip();
	//Update the frictionCoefficient
	void UpdateFrictionCoefficient(const float value) { FrictionCoefficient = value; }
	//Change the radius of the tire
	void UpdateTireRadius(const float value) { TireRadius = value; }
	//Calculates the maximum load the tire can bear
	void UpdateMaxTraction();

	//Updates the steering direction of the tire
	void UpdateSteering(const float NewAngle);
	//Update the vehicle fields the wheel has	
	void UpdateVehicleParameters(const float mass, const float wheelBaseLength, const float trackWidth, const float DistanceOfCGToFrontAxis, const float DistanceOfCGToRearAxis, const float CGHeight,const float NewGravity);
	//Store the contact location of the tire
	void StoreTireContactLocation(const  FVector NewHitLocation);
	//Update the suspension setting of the wheels
	void UpdateSuspension(const float Stiffness, const float Damping, const float SuspensionLength = 0.50f);
	//Apply deceleration to the wheel
	void ApplyBrakes(const float BrakeTorque, const float DeltaTime);
	//Gets the wheel's contact point
	FVector GetContactPoint() {return ContactPoint;}
	//Updates the current tire load
	void UpdateTireLoad(float NormalForce);
	//Get the wheel's tire load
	float GetTireLoad() const { return TireLoad; };
	//Updates the rotational velocity of the wheel
	float CalculateWheelRotationalVelocity(const float VehicleSpeed);
	//Updates the tire's slip ratio
	float CalculateSlipRatio(const float wheelSpeed);
	//Updates the tire's angle ratio
	float CalculateSlipAngle(const float velocityY, const float velocityX);
	//Gets the friction coefficient 
	float GetFrictionCoefficient() const { return FrictionCoefficient; }


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
	FVector ContactPoint = FVector::ZeroVector;
	float TireLoad = 0;
	bool IsGrounded = false;
private:

	float Gravity = 0;
	float VehicleMass = 300;
	float BaseTireLoad = 0;
	float VehicleWeight = 300000;
	float FrictionCoefficient = 1.4;
	float TireRadius = 1;
	float TrackWidth = 0;
	float SlipRatio = 0.0;
	float SlipAngle = 0.0;

	float MaximumWheelTraction = 0.5;
	float DistanceOfCentreOfGravityToTireAxis = 0.5;
	float WheelBase = 1;
	float CentreOfGravityHeight = 1;
	float WheelDamper = 0.98f;
	float CouplingFactor = 0.1f;
	float WheelRotationalVelocity = 0.0f;
	float WheelRotationalInertia = 0.5f;
	float LastBrakeTorque = 0.0f;
	float SuspensionForce = 0;

	float SuspensionCompression = 0.0f;
};
