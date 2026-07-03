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
	//Update the frictionCoefficient
	void UpdateFrictionCoefficient(const float value) { FrictionCoefficient = value; }
	//Change the radius of the tire
	void UpdateTireRadius(const float value) { TireRadius = value; }
	//Calculates the maximum load the tire can bear
	void UpdateMaxTireLoad();
	//Update the vehicle fields the wheel has	
	void UpdateVehicleParameters(const float mass, const float wheelBaseLength, const float DistanceOfCGToTireAxis, const float CGHeight);
	//Updates the wheel's suspension
	void UpdateWheelSuspension(const FVector NewSpringForce, const  FVector NewHitLocation);
	//Calculate the current tire load
	float UpdateTireLoad(const float Acceleration);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	bool IsFrontTire = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Position")
	FName SocketName;
	FWheelSuspensionSetting SuspensionSetting = FWheelSuspensionSetting();
private:
 
	float Gravity = 9.81;
	float VehicleMass = 300;
	float FrictionCoefficient = 0.5;
	float TireRadius = 1;
	float TireLoad = 0;
	float MaxTireLoad = 0.5;
	float DistanceOfCentreOfGravityToTireAxis = 0.5;
	float WheelBase = 1;
	float CentreOfGravityHeight = 1;
	FVector HitLocation = FVector::ZeroVector;
	FVector SpringForce = FVector::ZeroVector;
};
