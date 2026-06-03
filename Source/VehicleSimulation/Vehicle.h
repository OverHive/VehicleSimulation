// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputMappingContext.h"
#include "Vehicle.generated.h"

UCLASS()
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
	class UStaticMeshComponent* MeshComponent;

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MovementForce = 1000000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float mass = 500.0f; // High value because physics needs high force
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float ThrottleForce = 500000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float SteeringTorque = 20000000.0f;

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


private:
	// This function is called by the Enhanced Input System
	void Move(const FInputActionValue& Value);
	// Stores the input from the joystick/WASD
	FVector2D CurrentInputDirection;
	void Input_Throttle(const FInputActionValue& Value);
	void Input_Steering(const FInputActionValue& Value);
	void Input_Brake(const FInputActionValue& Value);

	float CurrentThrottle = 0.0f;
	float CurrentSteering = 0.0f;
	float CurrentBrake = 0.0f;

};
