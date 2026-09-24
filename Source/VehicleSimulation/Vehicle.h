// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputMappingContext.h"
#include "Tire.h"
#include "WheelSuspensionSetting.h"
#include "VehiclePresets.h"
#include "VehicleHUD.h"
#include "TelemetryLogger.h"
#include "Vehicle.generated.h"

UCLASS(BlueprintType, Blueprintable)
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
	//Called when the game ends or when despawned
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	//<----------------------------------------------- Menu functions----------------------------------------->
	UFUNCTION(BlueprintCallable, Category = "Preset")
	//Returns the vehicle to its initial orientation and position
	void ResetVehiclePosition();
	UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
	//Opens the menu
	void TogglePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	//Toggles the menu from a blueprint
	void BlueprintTogglePauseMenu();
	//Resets the state of the vehicle
	void ResetState();
	UFUNCTION(BlueprintCallable, Category = "Menu")
	//Gets the current index of the preset 
	float GetPresetIndex() { return PresetIndex; };

	//<----------------------------------------------- End ------------------------------------------------->

	//<----------------------------------------------- Preset variables ------------------------------------>
	//Applies a preset to the vehicle
	UFUNCTION(BlueprintCallable, Category = "Preset")
	void SetFromPreset(const int Index);
	//The stored presets
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preset")
	FVehiclePresets VehicleSettings;
	//The currently selected preset
	FCarSettings CurrentPresets;
	//The index of the selected preset
	int PresetIndex = 0;
	//<----------------------------------------------- End ------------------------------------------------->

	//<----------------------------------------------- Mesh ------------------------------------------------>
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* SkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* PhysicsMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* VisualMesh;
	//<----------------------------------------------- End ------------------------------------------------->

	//<----------------------------------------------- Input Assets ---------------------------------------->
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
	UInputAction* PauseAction;
	//<----------------------------------------------- End ------------------------------------------------->

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float AirDensity = 1.20;
	//The threshold for activating the magic formula
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float FormulaThreshold = 5.0f; //Km/h
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle Parameters");
	float GearRatio = 3.0f;
	//Vehicle steering
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	float MaxSteeringAngle = 35.0f;      // degrees at full lock

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	float SteeringInterpSpeed = 20.0f;


	//<----------------------------------------------- Vehicle's tire------------------------------------------>

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
	//<----------------------------------------------- End --------------------------------------------------->
	UPROPERTY(EditAnywhere, Category = "Suspension")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float MaxSuspensionLength = 50.0f;//cm
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float LongitudinalDamping = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	float SpringStiffness = 8.0f;
private:
	//<----------------------------------------------- Vehicle state updaters -------------------------------->
	//Models vehicle suspension using ray cast
	void SuspensionRayCast();
	//Setup for the wheels
	void CreateTires();
	//Handles the dynamics of the suspension
	void CalculateSuspensionDynamics(float DeltaTime, float LongitudionalAcceleration, const float LateralAcceleration);
	//Calculates the resistive force
	void CalculateResistiveForces(FVector Velocity, float DeltaTime);
	//Calculates pitch weight transfer effects
	void CalculateWeightTransfer(const float LongitudionalAcceleration, const float LateralAcceleration);
	//Handles unsprung mass dynamics
	void CalculateUnsprungMassDynamics(float DeltaTime);
	// Handles pitch dynamics
	void CalculatePitchAndHeaveDynamics(float DeltaTime, float LongitudinalAcceleration);
	//Updates the state of a wheel
	void UpdateWheel(UTire* Tire, const float LongitudinalForceMagnitude, float DeltaTime);
	//Applies suspension forces to the vehicle
	void ApplySuspensionForceEffects();
	//Apply a force through a wheel
	void ApplyWheelForce(UTire* Tire, float ForceMagnitude, FVector Direction, const bool HasXTorque = false, const bool HasYTorque = false, const bool HasZTorque = false);
	//Applies a force at location
	void ApplyLocationForce(FVector Force, FVector Position, const bool HasZTorque = false, const bool HasYTorque = false, const bool HasXTorque = false);
	//Obtains the static distribution of the vehicle's weight
	void UpdateStaticLoads();
	//Get throttle from a engine 
	float GetDriveForce(float Torque) const;
	//Gets the traction force on for a wheel
	float GetTireDriveForce(UTire* Tire);
	//Calculates the maximum braking force base on the available slip
	float GetMaximumSlipBasedBreakingForce(UTire* Tire, float DeltaTime);
	//Gets the absolute braking force for a wheel
	float GetUnSignedTireBrakingForce(UTire* Tire, float DeltaTime);
	//Gets the resistive force on a Tire
	float GetTireRollingResistance(UTire* Tire, FVector WheelDirection, float DeltaTime);
	//Calculates the minimum stopping force on a wheel
	float GetMinimumWheelForce(UTire* Tire, const float BrakingkingForce, FVector WheelDirection, const float DeltaTime);
	//Rounds a float to a given number of decimal points
	float RoundToDecimalPoint(const float Value, const int Points = 3);
	//Gets the current rotations per minute of a give tire
	float CalculateRPM(UTire* Tire) const;
	//Gets the drive torque on a wheel
	float GetWheelTorque(UTire* Tire) const;
	//Creates Torque for the vehicle 
	FVector CreateVehicleTorque(FVector Force, FVector Position);
	//<----------------------------------------------- End --------------------------------------------------->

	//<----------------------------------------------- Input data and function ------------------------------->
	FVector2D CurrentInputDirection;
	void Input_Throttle(const FInputActionValue& Value);
	void Input_Steering(const FInputActionValue& Value);
	void Input_Brake(const FInputActionValue& Value);
	void Input_Gear(const FInputActionValue& Value);
	void Input_Pause(const FInputActionValue& Value);
	//For preventing multiple gear changes a single press 
	bool IsGearChanging = false;
	//<----------------------------------------------- End --------------------------------------------------->

	//<-----------------------------------------------Vehicle state ------------------------------------------>
	int GearIndex = 0;
	float FrontSuspensionForce = 0.0f;
	float RearSuspensionForce = 0.0f;
	float CurrentThrottle = 0.0f;
	float CurrentSteering = 0.0f;
	float CurrentBrake = 0.0f;
	float CurrentSteeringAngle = 0.0f;
	float StaticFrontLoad = 0.0f;
	float StaticRearLoad = 0.0f;
	float VehicleWeight = 0.0f;
	float VehicleTorque = 0;
	float FrontUnsprungPosition = 0.0f;
	float FrontUnsprungVelocity = 0.0f;
	float RearUnsprungPosition = 0.0f;
	float RearUnsprungVelocity = 0.0f;
	float FrontUnsprungForce = 0.0f;
	float RearUnsprungForce = 0.0f;
	float CurrentDrivingForce = 0.0f;
	float CurrentDrag = 0.0f;
	float PitchAngle = 0.0f;
	float PitchVelocity = 0.0f;
	float HeavePosition = 0.0f;
	float HeaveVelocity = 0.0f;
	float FrontDynamicLoad = 0.0f;
	float RearDynamicLoad = 0.0f;
	float TargetHeight = 0.0f;
	bool IsUsingMagicFormula = false;
	bool IsBraking = false;
	bool IsPauseButtonDown = false;
	FVector CurrentVelocity = FVector::ZeroVector;
	FVector LastVelocity = FVector::ZeroVector;
	FVector Acceleration = FVector::ZeroVector;
	//<----------------------------------------------- End ------------------------------------------------->

	//<-----------------------------------------------Spawning data ---------------------------------------->
	FVector SpawnLocation = FVector::ZeroVector;
	FQuat SpawnDirection = FQuat(0, 0, 0, 0);
	//<----------------------------------------------- End ------------------------------------------------->

	//<-----------------------------------------------Custom HUD ------------------------------------------->
	VehicleHUD HUDDisplay;
	//Refreshes the HUD
	void UpdateHUD();
	//<----------------------------------------------- End ------------------------------------------------->

	//<----------------------------------------------- Telemetry logging ----------------------------------->
	TelemetryLogger VehicleLogger;
	//Logs vehicle telemetry
	void LogTelemetry();
	// How often to log (in seconds)
	UPROPERTY(EditAnywhere, Category = "CSV Settings")
	float LogInterval = 1.0f;
	// Timer handle to manage the loop
	FTimerHandle TimerHandle;
	float TimeBetweenLastFrames = 0.0f;
	//<----------------------------------------------- End ------------------------------------------------->

	//<----------------------------------------------- Mesh Resizing data ---------------------------------->

	FVector MeshDimension = FVector::ZeroVector;
	FVector DefaultVisualMeshPosition = FVector::ZeroVector;
	FVector MeshScale = FVector(1, 1, 1);
	//<----------------------------------------------- End ------------------------------------------------->


};
