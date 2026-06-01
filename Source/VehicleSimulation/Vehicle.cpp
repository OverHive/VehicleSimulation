// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AVehicle::AVehicle()
{
    // Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
    //Create the Root Component
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // Enable physic and gravity;
    MeshComponent->SetSimulatePhysics(true);
    MeshComponent->SetEnableGravity(true);
    MeshComponent->SetMassOverrideInKg("true", mass);
}

// Called when the game starts or when spawned
void AVehicle::BeginPlay()
{
    Super::BeginPlay();
    // Add the Mapping Context
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

}

// Called every frame
void AVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Apply force each frame based on stored input
    if (MeshComponent && !CurrentInputDirection.IsNearlyZero())
    {
        // Calculate direction based on Actor orientation
        FVector Forward = GetActorForwardVector();
        FVector Right = GetActorRightVector();

        // Combine directions with input (Y is Forward/Backward, X is Left/Right)
        FVector MoveDirection = (Forward * CurrentInputDirection.Y) + (Right * CurrentInputDirection.X);

        // Apply Force
        MeshComponent->AddForce(MoveDirection * MovementForce);
    }

}

// Called to bind functionality to input
void AVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    //Bind the Enhanced Input Action
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVehicle::Move);

        //Reset input when key is released
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AVehicle::Move);
    }

}

void AVehicle::Move(const FInputActionValue& Value)
{
    // Store the 2D vector from the input action
    CurrentInputDirection = Value.Get<FVector2D>();
    keyDown = true;
    // Set the input to the default position if the key is released
    if (CurrentInputDirection.IsNearlyZero())
    {
        CurrentInputDirection = FVector2D::ZeroVector;
    }
}