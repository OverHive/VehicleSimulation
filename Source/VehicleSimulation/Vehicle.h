// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputMappingContext.h"
#include "Tire.h"
#include "WheelSuspensionSetting.h"
#include "VehiclePresets.h"
#include "Vehicle.generated.h"
enum Axis { BACK, FRONT };


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
	//Applies a preset to the vehicle
	void SetFromPreset(const float Index);

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* SkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class  UStaticMeshComponent* PhysicMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class  UStaticMeshComponent* VisualMesh;
	//Input Assets
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* VehicleMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ThrottleAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SteeringAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* BrakeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* GearAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* DebugAction;

	// --- Vehicle parameters ---
	float TotalVehicleMass = 190.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float VehicleSprungMass = 150.0f; // kg
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
	float TrackWidth = 140.0f;//cm/s
	//The threshold for activating the magic formula
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float FormulaThreshold = 5.0f; //Km/h
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float GearRatio = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float FinalDriveRatio = 4.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float DrivetrainEfficiency = 0.9;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	TArray<float> GearRatios = { 0.8 };
	//Vehicle steering
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	float MaxSteeringAngle = 35.0f;      // degrees at full lock

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	float SteeringInterpSpeed = 20.0f;
	//The vehicle's tires and their properties
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	float FrontWheelRadius = 30.0f; //cm
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	float RearWheelRadius = 30.0f; //cm
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	float FrontTireVerticalStiffness = 20000000.0f;  // N/cm
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	float RearTireVerticalStiffness = 20000000.0f;  // N/cm
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	float FrontTireFriction = 1.2;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	float RearTireFriction = 1.2;

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
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Tires")
	TArray<UTire*> FrontTires;

	UPROPERTY(EditAnywhere, Category = "Suspension")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float PitchInertia = 780000.0f; // kg·cm²f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float FrontSuspensionStiffness = 3500.0f;// Kg/cm

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float RearSuspensionStiffness = 2800.0f;// Kg/cm

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float FrontSuspensionDamping = 3000.0f;// Ns/cm

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float RearSuspensionDamping = 2500.0f;//Ns/cm

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float FrontUnsprungMass = 10.0f;//Kg

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float RearUnsprungMass = 10.0f;//Kg

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float MaxSuspensionLength = 50.0f;//cm
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float FrontUnSprungDamping = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float RearUnSprungDamping = 0.95f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float PitchDamping = 0.95f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float PitchStiffness = 5000.0f;//N/cm
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float HeaveDamping = 0.98f;
	FName VehiclePresetName = "Undefined";
private:
	//Models vehicle suspension using ray cast
	void SuspensionRayCast();
	// This function is called by the Enhanced Input System
	void Move(const FInputActionValue& Value);
	//Handles braking
	void ApplyBraking(UTire* Tire, const float VehicleSpeedAtWheel, float DeltaTime);
	//Setup for the tires
	void CreateTires();
	//Handles the dynamics of the suspension
	void CalculateSuspensionDynamics(float DeltaTime, float LongitudionalAcceleration, const float LateralAcceleration);
	//Calculates the resistive force
	void CalculateResistiveForces(FVector Velocity, float DeltaTime);
	//Calculates pitch weight transfer effects
	void CalculatePitchWeightTransfer(const float LongitudionalAcceleration, const float LateralAcceleration);
	//Handles unsprung mass dynamics
	void CalculateUnsprungMassDynamics(float DeltaTime);
	// Handles pitch dynamics
	void CalculatePitchAndHeaveDynamics(float DeltaTime, float LongitudinalAcceleration);
	//Updates the state of a wheel
	void UpdateWheel(UTire* Tire, float DeltaTime);
	//Applies suspension forces to the vehicle
	void ApplySuspensionForceEffects();
	//Apply a force through a wheel
	void ApplyWheelForce(UTire* Tire, float ForceMagnitude, FVector Direction, bool IsForward);
	//Applies a force at location
	void ApplyLocationForce(FVector Force, FVector Position, bool HasPitch = false);
	//Obtains the static distribution of the vehicle's weight
	void UpdateStaticLoads();
	//Get throttle from a engine 
	float GetDriveForce(float Torque) const;
	//Gets the traction force on for a wheel
	float GetTireTraction(UTire* Tire);
	//Gets the braking force for a wheel
	float GetTireBrakingForce(UTire* Tire, float DeltaTime);
	//Gets the resistive force on a Tire
	float GetTireRollingResistance(UTire* Tire, float DeltaTime);
	//Calculates the minimum stopping force on a wheel
	float GetMinimumWheelForce(UTire* Tire, const float BrakingkingForce, const float DeltaTime);
	//Rounds a float to a given number of decimal points
	float RoundToDecimalPoint(const float Value, const int Points = 3);
	//Gets the current rotations per minute of a give tire
	float CalculateRPM(UTire* Tire) const;
	//Gets the drive torque on a wheel
	float GetWheelTorque(UTire* Tire) const;

	//Note: Forces must be in  cm/s²
	float FrontSuspensionForce = 0.0f;
	float RearSuspensionForce = 0.0f;
	// Stores the input from the joystick/WASD
	FVector2D CurrentInputDirection;
	void Input_Throttle(const FInputActionValue& Value);
	void Input_Steering(const FInputActionValue& Value);
	void Input_Brake(const FInputActionValue& Value);
	void Input_Debug(const FInputActionValue& Value);
	void Input_Gear(const FInputActionValue& Value);
	FVector CurrentVelocity = FVector::ZeroVector;
	FVector LastVelocity = FVector::ZeroVector;
	FVector Acceleration = FVector::ZeroVector;
	FVector DefaultVisualMeshPosition = FVector::ZeroVector;
	int GearIndex = 0;
	//Debug configuration
	float DebugSetting = 0.0f;
	float DragPrintSpeedThreshold = 1.0f;
	float LastFakeAcceleration = 0.0f;
	float FakeAcceleration = -5.0f;
	float CurrentThrottle = 0.0f;
	float CurrentSteering = 0.0f;
	float CurrentBrake = 0.0f;
	float CurrentSteeringAngle = 0.0f;
	float StaticFrontLoad = 0.0f;
	float StaticRearLoad = 0.0f;
	float VehicleWeight = 0.0f;
	float VehicleTorque = 0;
	float ZOffset = 8.0f;
	float FrontUnsprungPosition = 0.0f;
	float FrontUnsprungVelocity = 0.0f;
	float RearUnsprungPosition = 0.0f;
	float RearUnsprungVelocity = 0.0f;
	float FrontUnsprungForce = 0.0f;
	float RearUnsprungForce = 0.0f;
	float CurrentDrivingForce = 0.0f;
	float CurrentDrag = 0.0f;
	float ThrottleForce = 150000.0f;//cN
	float CurrentBrakingForce = 0.0f;//cN
	TMap <float, float> TorqueCurve;
	float CurveStep = 0;
	float MaximumRPM = 0;
	float MinimumStartingRPM = 0;
	float PitchAngle = 0.0f;
	float PitchVelocity = 0.0f;
	float HeavePosition = 0.0f;
	float HeaveVelocity = 0.0f;
	float FrontWheelInertia = 0.0f;
	float RearWheelInertia = 0.0f;
	float FrontDynamicLoad = 0.0f;
	float RearDynamicLoad = 0.0f;
	float FrontRollingResistanceCoefficient = 0.0f;
	float RearRollingResistanceCoefficient = 0.0f;
	float AntiDiveFactor = 1.0f;
	bool IsBraking = false;
	bool IsLongitudinalControlled = true;


	float TargetHeight = 30.0f;

	float SpringStiffness = 5000.0f;

	float Damping = 500.0f;

	//For preventing multiple gear changes a single press 
	bool IsGearChanging = false;
	VehiclePresets VehicleSettings;
	TArray<FName> socketNames{ "Socket_FR","Socket_FL","Socket_RR","Socket_RL" };

};
