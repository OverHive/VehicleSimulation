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
    MeshComponent->SetAngularDamping(1.0f);

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
            Subsystem->AddMappingContext(VehicleMappingContext, 0);
        }
    }

}

// Called every frame
void AVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Apply force each frame based on stored input
    if (MeshComponent)
    {
        // Throttle
        FVector ForwardForce = GetActorForwardVector() * (CurrentThrottle * ThrottleForce);
        //Calculate drag using: Drag force = 0.5*drag Coefficient*Area*air density* speed^2
        FVector Velocity = MeshComponent->GetPhysicsLinearVelocity();
        float CurrentSpeed = Velocity.Size();
        float Area = Height * Width;
        FVector DragForce = Velocity.GetSafeNormal() *0.5* DragCoefficient*Area* AirDensity * CurrentSpeed * CurrentSpeed;
        //Subtract resistive forces from the diving force
        ForwardForce -= DragForce;
        MeshComponent->AddForce(ForwardForce, NAME_None, false);

        // Steering (change to incorporate wheel in future)
        FVector Torque = GetActorUpVector() * (CurrentSteering * SteeringTorque);

        MeshComponent->AddTorqueInDegrees(Torque, NAME_None, false);
        if (!Torque.IsNearlyZero())
        {
            FMessageLog("Game").Info(FText::FromString((GetActorUpVector() * CurrentSteering).ToCompactString()));
        }
        // Braking
        if (CurrentBrake > 0.0f)
        {
            FVector BrakingForce = -MeshComponent->GetPhysicsLinearVelocity().GetSafeNormal() * (CurrentBrake * BrakeForce);
            MeshComponent->AddForce(BrakingForce, NAME_None, false);
        }
    }

}

// Called to bind functionality to input
void AVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    //Bind the Enhanced Input Action
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Bind Throttle
        EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AVehicle::Input_Throttle);
        EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AVehicle::Input_Throttle);

        // Bind Steering
        EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &AVehicle::Input_Steering);
        EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &AVehicle::Input_Steering);

        // Bind Brake
        EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &AVehicle::Input_Brake);
        EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &AVehicle::Input_Brake);
    }

}

void AVehicle::Input_Throttle(const FInputActionValue& Value)
{
    CurrentThrottle = Value.Get<float>();
}

void AVehicle::Input_Steering(const FInputActionValue& Value)
{
    CurrentSteering = Value.Get<float>();
}

void AVehicle::Input_Brake(const FInputActionValue& Value)
{
    CurrentBrake = Value.Get<float>();
}