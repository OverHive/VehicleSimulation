// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h" 

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
    //  Create the Anchors
    FL_Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("FL_Anchor"));
    AllAnchors.Add(FL_Anchor);

    FR_Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("FR_Anchor"));
    AllAnchors.Add(FR_Anchor);
    RR_Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("RR_Anchor"));
    AllAnchors.Add(RR_Anchor);
    RL_Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("RL_Anchor"));
    AllAnchors.Add(RL_Anchor);


    //Create the tires
    FrontLeftTire = CreateDefaultSubobject<UTire>(TEXT("FrontLeftTire"));    
    AllTires.Add(FrontLeftTire);
    FrontRightTire = CreateDefaultSubobject<UTire>(TEXT("FrontRightTire"));
    AllTires.Add(FrontRightTire);
    RearRightTire = CreateDefaultSubobject<UTire>(TEXT("RearRightTire"));
    AllTires.Add(RearRightTire);
    RearLeftTire = CreateDefaultSubobject<UTire>(TEXT("RearLeftTire")); 
    AllTires.Add(RearLeftTire);
    //Add the tires

    
    
   
    for (int i = 0; i < 4; i++)
    {
        //Connect the anchor to the vehicle mesh
        AllAnchors[i]->SetupAttachment(MeshComponent);
        //... and connect the corresponding tire to the anchor
        AllTires[i]->SetupAttachment(AllAnchors[i]);
    }
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
        //Apply force through cent
        FVector CoM = MeshComponent->GetCenterOfMass();
        MeshComponent->AddForceAtLocation(ForwardForce, CoM);
   

        //// Steering (change to incorporate wheel in future)
        FVector Torque = GetActorUpVector() * (CurrentSteering * SteeringTorque);

        MeshComponent->AddTorqueInDegrees(Torque, NAME_None, false);
        //
        // Braking
        if (CurrentBrake > 0.0f)
        {
            FVector BrakingForce = -MeshComponent->GetPhysicsLinearVelocity().GetSafeNormal() * (CurrentBrake * BrakeForce);
            MeshComponent->AddForce(BrakingForce, NAME_None, false);
        }
        //Update the HUD parameters

        CurrentVelocity = CurrentSpeed;
        //Calculate acceleration with acceleration = change in velocity/change in time
        Acceleration = (CurrentVelocity - LastVelocity) / DeltaTime;
        //Store the current velocity of the next frame
        LastVelocity = CurrentVelocity;
        if (GEngine)
        {
            
            GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Green, FString::Printf(TEXT("Direction %s"), *GetActorForwardVector().ToString()));
            GEngine->AddOnScreenDebugMessage(2, 5.f, FColor::Green, FString::Printf(TEXT("Speed %f"), CurrentVelocity));
            GEngine->AddOnScreenDebugMessage(3, 5.f, FColor::Green, FString::Printf(TEXT("Acceleration %f"), Acceleration));
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