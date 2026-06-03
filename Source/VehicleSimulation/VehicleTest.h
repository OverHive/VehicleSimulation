// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputMappingContext.h"
#include "VehicleTest.generated.h"

UCLASS()

class VEHICLESIMULATION_API AVehicleTest : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVehicleTest();

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

	// --- Input Assets (Assign these in the Blueprint Editor) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	// --- Vehicle parameters ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MovementForce = 1000000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float mass = 50.0f; // High value because physics needs high force



private:
	// This function is called by the Enhanced Input System
	void Move(const FInputActionValue& Value);
	// Stores the input from the joystick/WASD
	FVector2D CurrentInputDirection;
	bool keyDown = false;

};
