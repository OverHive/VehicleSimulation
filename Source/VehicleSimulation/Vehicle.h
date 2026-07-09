// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputMappingContext.h"
#include "Tire.h"
#include "WheelSuspensionSetting.h"
#include "Vehicle.generated.h"



UCLASS()
//Note
class VEHICLESIMULATION_API AVehicle : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVehicle();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* SkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class  UStaticMeshComponent* MeshComponent;

	//Input Assets
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* VehicleMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ThrottleAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SteeringAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* BrakeAction;

	// --- Vehicle parameters ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float VehicleMass = 150.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float ThrottleForce = 30000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float BrakeForce = 300000.0f;
	//For calculating drag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float Width = 1.00;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float Height = 1.00;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float AirDensity = 1.20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float DragCoefficient = 0.30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float WheelBaseLength = 107.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float CentreOfGravityHeight = 30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float DistanceOfCentreOfGravityToFrontAxis = 59.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float DistanceOfCentreOfGravityToRearAxis = 45.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float FinalGearRatio = 5.6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float TrackWidth = 140.0f;
	//Vehicle steering
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	float MaxSteeringAngle = 35.0f;      // degrees at full lock
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	float AngularDampingWhenSteeringReleased = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	float SteeringReleaseThreshold = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	float SteeringInterpSpeed = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	float SelfAligningTorqueCoefficient = 5000.0f;  // Caster effect strength

	//The vehicle's tires 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	UTire* FrontLeftTire;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	UTire* FrontRightTire;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	UTire* RearLeftTire;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	UTire* RearRightTire;
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	TArray<UTire*> AllTires;

	UPROPERTY(EditAnywhere, Category = "Suspension")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

private:
	//Models vehicle suspension using ray cast
	void SuspensionRayCast();
	// This function is called by the Enhanced Input System
	void Move(const FInputActionValue& Value);
	//Setup for the tires
	void CreateTires();
	//Calculates the resistive force
	void CalculateResistiveForce(FVector Velocity);
	// Stores the input from the joystick/WASD
	FVector2D CurrentInputDirection;
	void Input_Throttle(const FInputActionValue& Value);
	void Input_Steering(const FInputActionValue& Value);
	void Input_Brake(const FInputActionValue& Value);
	FVector CurrentVelocity = FVector::ZeroVector;
	FVector LastVelocity = FVector::ZeroVector;
	float CurrentThrottle = 0.0f;
	float CurrentSteering = 0.0f;
	float CurrentBrake = 0.0f;
	float CurrentMass = 500.0f;
	float CurrentSteeringAngle = 0.0f;
	TArray<FName> socketNames{ "Socket_FR","Socket_FL","Socket_RR","Socket_RL" };

};
