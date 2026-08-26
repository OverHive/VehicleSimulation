// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "WheelSuspensionSetting.h"
#include "Tire.generated.h"

static float Gravity = 980.0f;
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
	//Gets the normal force on the tire
	float GetNormalForce() const;
	//Gets the current suspension force
	float GetSuspensionForce() { return IsGrounded ? SuspensionForce : 0; }
	//Get the wheel's tire load
	float GetTireLoad() const { return TireLoad; };
	//Gets the friction coefficient 
	float GetFrictionCoefficient() const { return FrictionCoefficient; }
	//Model the force using slip
	float MagicFormula(const float value, const float x) const;
	//Returns the tire's slip angle
	float GetSlipAngle() const { return SlipAngle; };
	//Returns the rolling radius of wheel
	float GetRollingRadius() { return RollingRadius; };
	//Returns the current longitudinal force on the tire
	float GetCurrentLongitudinalForceOnTire() const {
		return CurrentLongitudinalForceOnTire;
	}
	//Gets the lateral grip of the tire
	const float GetLateralGrip() const;
	//Applies the friction circle to a given force
	float FrictionCircle(const float LongitudinalForce, const float LaterialForce, const bool LongitudinalLeading) const;
	//Returns the distance the tire is from the ground
	float GetDistanceFromGround() { return DistanceFromGround; };
	//Update the frictional coefficient of the tire
	void UpdateTireFrictionCoefficient(const float NewValue);
	//Update the friction coefficient
	void UpdateFrictionCoefficient(const float NewValue);
	//Update the rolling resistance coefficient
	void UpdateRollingResistanceCoefficient(const float NewValue) { RollingResistanceCoefficient = NewValue; }
	//Change the radius of the tire
	void UpdateTireRadius(const float value) { SuspensionSettings.WheelRadius = value; }
	//Calculates the maximum load the tire can bear
	void UpdateMaxGrip();
	//Updates the rotational velocity of the wheel
	void UpdateWheelRotationalVelocity(const float NetTorque, const float DeltaTime, const bool IsForward );
	//Updates the steering direction of the tire
	void UpdateSteering(const float NewAngle);
	//Store the contact location of the tire
	void StoreTireContactInformation(const  FHitResult  NewHitLocation);
	//Update the suspension setting of the wheels
	void UpdateSuspension(const float Stiffness, const float Damping, const float SuspensionLength, const float TireStiffness, const float UnSprungMass, const float StaticTireLoad);
	//Updates the tire's slip ratio
	void UpdateSlipRatio(const float VehicleSpeed, const bool IsBraking);
	//Updates the current tire load
	void UpdateTireLoad(float NormalForce);
	//Updates the tire's angle ratio
	void UpdateSlipAngle(const float velocityY, const float velocityX);
	//Calculates the rolling radius of the wheel
	void UpdateRollingRadius(FVector AxisPosition);
	//Updates the longitudinal force on the tire
	void UpdateLongitudinalForce(const float NewForce) { CurrentLongitudinalForceOnTire += NewForce; }
	//Update the wheel's Inertia
	void UpdateWheelInertia(float NewInertia) { Inertia = NewInertia; };
	//Resets the force on the tire
	void ResetForces() { CurrentLongitudinalForceOnTire = 0; };

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
	FWheelSuspensionSetting SuspensionSettings = FWheelSuspensionSetting();

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
	float CurrentLongitudinalForceOnTire = 0.0f;
	float Inertia = 8500; //Kg/ cm^2

	float TireCompression = 0.0f;
	float SuspensionCompression = 0.0f;
	float DistanceFromGround = 0.0f;
};
