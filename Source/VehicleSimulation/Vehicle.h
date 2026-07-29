// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputMappingContext.h"
#include "Tire.h"
#include "WheelSuspensionSetting.h"
#include "Vehicle.generated.h"
enum Axis {BACK,FRONT};


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
	float VehicleMass = 150.0f; // kg
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float ThrottleForce = 1500000.0f;//cN

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float MaxBrakeTorque = 50000.0f;//N/cm
	float BrakeForce = 1000000.0f;//cN
	//For calculating drag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float Width = 210.0f;//cm
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float Height = 150.0f;//cm
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float AirDensity = 1.20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float DragCoefficient = 0.30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float WheelBaseLength = 107.0f;//cm/s
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float CentreOfGravityHeight = 30.0f;//cm/s
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float DistanceOfCentreOfGravityToFrontAxis = 59.0f;//cm/s
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float DistanceOfCentreOfGravityToRearAxis = 48.0f;//cm/s
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float FinalGearRatio = 5.6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float TrackWidth = 140.0f;//cm/s
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float OptimalBrakingSlip = 0.18f;
	//The threshold for activating the magic formula
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float FormulaThreshold = 5.0f; //Km/h
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
	float SelfAligningTorqueCoefficient = 5000.0f;  // cm/N

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float PitchInertia = 86.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float FrontSuspensionStiffness = 35000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float RearSuspensionStiffness = 28000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float FrontSuspensionDamping = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float RearSuspensionDamping = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float FrontUnsprungMass = 50.0f;//Kg

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float RearUnsprungMass = 50.0f;//Kg

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float SuspensionRestLength = 50.0f;//cm
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float PitchDamping = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float HeaveDamping = 0.98f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float AntiDivePercentage = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float AntiSquatPercentage = 0.25f;    

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float AntiDiveAngle = 20.0f;
private:
	//Models vehicle suspension using ray cast
	void SuspensionRayCast();
	// This function is called by the Enhanced Input System
	void Move(const FInputActionValue& Value);
	//Handles braking
	void ApplyBraking(UTire * Tire, const float VehicleSpeedAtWheel, FVector WheelForward, float DeltaTime);
	//Setup for the tires
	void CreateTires();
	//Handles the dynamics of the suspension
	void CalculateSuspensionDynamics(float DeltaTime, float LongitudionalAcceleration, const float LateralAcceleration);
	//Calculates the resistive force
	void CalculateResistiveForce(FVector Velocity);
	//Calculates pitch weight transfer effects
	void CalculatePitchWeightTransfer(const float LongitudionalAcceleration, const float LateralAcceleration);
	// Calculate pitch dynamics
	void CalculatePitchAndHeaveDynamics(float DeltaTime,float LongitudinalAcceleration);
	//Updates the state of a wheel
	void UpdateWheel(UTire* Tire);
	//Applies suspension forces to the vehicle
	void ApplySuspensionForceEffects();
	//Apply a force through a wheel
	void ApplyWheelForce (UTire* Tire, float ForceMagnitude, FVector Direction);
	//Applies a force at location
	void ApplyLocationForce(FVector Force,FVector Position);

	//Note: Forces must be in  cm/s²
	float FrontSuspensionForce = 0.0f;
	float RearSuspensionForce = 0.0f;
	float Gravity = 981;
	// Stores the input from the joystick/WASD
	FVector2D CurrentInputDirection;
	void Input_Throttle(const FInputActionValue& Value);
	void Input_Steering(const FInputActionValue& Value);
	void Input_Brake(const FInputActionValue& Value);
	FVector CurrentVelocity = FVector::ZeroVector;
	FVector LastVelocity = FVector::ZeroVector;
	FVector Acceleration = FVector::ZeroVector;
	float CurrentThrottle = 0.0f;
	float CurrentSteering = 0.0f;
	float CurrentBrake = 0.0f;
	float CurrentSteeringAngle = 0.0f;

	float PitchAngle = 0.0f;
	float PitchVelocity = 0.0f;
	float HeavePosition = 0.0f;
	float HeaveVelocity = 0.0f;
	float FrontDynamicLoad = 0.0f;
	float RearDynamicLoad = 0.0f;
	float AntiDiveFactor = 1.0f;
	bool Initialized = true;
	bool IsBraking = false;
	TArray<FName> socketNames{ "Socket_FR","Socket_FL","Socket_RR","Socket_RL" };

};
