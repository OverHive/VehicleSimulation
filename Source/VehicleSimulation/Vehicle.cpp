// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h" 
#include <warning.h>

// Sets default values
AVehicle::AVehicle()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//Create the Root Component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleComponent"));

	if (SkeletalMesh && MeshComponent)
	{
		RootComponent = MeshComponent;
		SkeletalMesh->SetupAttachment(MeshComponent);

		MeshComponent->SetAngularDamping(1.0f);

		// Enable physic and gravity;
		MeshComponent->SetSimulatePhysics(true);
		MeshComponent->SetEnableGravity(true);
		MeshComponent->SetMassOverrideInKg(NAME_None, VehicleMass, true);

		//Create the front tires

		FrontRightTire = CreateDefaultSubobject<UTire>(TEXT("Front-Right Tire"));
		if (FrontRightTire)
			FrontRightTire->IsFrontTire = true;


		FrontLeftTire = CreateDefaultSubobject<UTire>(TEXT("Front-Left Tire"));
		if (FrontLeftTire)
			FrontLeftTire->IsFrontTire = true;

		//Create the back tires
		RearRightTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Right Tire"));


		RearLeftTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Left Tire"));
		CreateTires();
	}
}

// Called when the game starts or when spawned
void AVehicle::BeginPlay()
{
	MeshComponent->SetMassOverrideInKg(NAME_None, VehicleMass, true);
	Super::BeginPlay();
	// Add the Mapping Context
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (VehicleMappingContext)
			{
				Subsystem->AddMappingContext(VehicleMappingContext, 0);
			}
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
		FVector DragForce = Velocity.GetSafeNormal() * 0.5 * DragCoefficient * Area * AirDensity * CurrentSpeed * CurrentSpeed;
		//Subtract resistive forces from the diving force
		ForwardForce -= DragForce;
		//Apply the force on the vehicle

		MeshComponent->AddForce(ForwardForce, NAME_None, false);

		// Steering (change to incorporate wheel in future)
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
		Acceleration = ((CurrentVelocity - LastVelocity) / DeltaTime) * 0.01;
		//Store the current velocity of the next frame
		LastVelocity = CurrentVelocity;
		//Display the settings
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Green, FString::Printf(TEXT("Speed %f km/h"), CurrentVelocity * 0.036));
			GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Green, FString::Printf(TEXT("Acceleration %f m/s^2"), Acceleration));

			float Index = 4;
			for (int i = 0; i < AllTires.Num(); i++)
			{
				if (AllTires[i])
				{
					float TireLoad = AllTires[i]->UpdateTireLoad(Acceleration);
					GEngine->AddOnScreenDebugMessage(3 + i, 3.f, FColor::Green, FString::Printf(TEXT("%s's tire load :%f N"), *AllTires[i]->GetName(), TireLoad));
				}
			}
		}
		//
		SuspensionRayCast();
	}

}
void AVehicle::SuspensionRayCast()
{
		for ( UTire* &Tire:AllTires)
		{

			//Get the Wheel and its corresponding ray
			const FWheelSuspensionSetting& Wheel = Tire->SuspensionSetting;
			FVector StartLocation = SkeletalMesh->GetSocketLocation(Tire->SocketName);

			//Vehicle up direction
			FVector VehicleUpDirection = MeshComponent->GetUpVector();

			// The ray points downwards relative to the vehicle's orientation

			FVector RayDirection = VehicleUpDirection * -1.0f;
			FVector EndLocation = StartLocation + (RayDirection * (Wheel.SuspensionLength + Wheel.WheelRadius));

			FHitResult Hit;
			FCollisionQueryParams QueryParameters;
			QueryParameters.AddIgnoredActor(this);

			//Perform the Ray cast
			if (GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, TraceChannel, QueryParameters))
			{
				//Calculate compression using distance from mount point to the ground hit point
				float CurrentDistance = FVector::Dist(StartLocation, Hit.Location);

				// Calculate the spring compression. We subtract WheelRadius because the "ground" hit is at the bottom of the tire
				float Compression = Wheel.SuspensionLength - (CurrentDistance - Wheel.WheelRadius);

				// Clamp compression to prevent negative values/extreme forces
				Compression = FMath::Max(0.0f, Compression);

				//Calculate Spring Force (Hooke's Law)
				float SpringForceMagnitude = Compression * Wheel.SpringStiffness;

				// Calculate Damping Force
				// We need the velocity of the vehicle at the specific point where the suspension is attached
				FVector VelocityAtPoint = MeshComponent->GetPhysicsLinearVelocityAtPoint(StartLocation);

				// The damping force is based on the velocity along the suspension axis (Up Vector)
				float SuspensionVelocity = FVector::DotProduct(VelocityAtPoint, VehicleUpDirection);
				float DampingForceMagnitude = SuspensionVelocity * Wheel.DampingCoefficient;

				//Combine Forces
				// Total Force = Spring - Damping (Damping opposes the velocity)
				float TotalForceMagnitude = SpringForceMagnitude - DampingForceMagnitude;

				// Apply the force upwards along the vehicle's local up vector
				FVector TotalForceVector = VehicleUpDirection * TotalForceMagnitude;

				//Apply Force to the Physics Body
				// We apply it at the hit location to create torque/rotation naturally
				MeshComponent->AddForceAtLocation(TotalForceVector, Hit.Location);
				//Update the wheel's suspension
				Tire->UpdateWheelSuspension(TotalForceVector, Hit.Location);
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
		if (ThrottleAction)
		{
			EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AVehicle::Input_Throttle);
			EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AVehicle::Input_Throttle);
		}
		if (SteeringAction)
		{
			// Bind Steering
			EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &AVehicle::Input_Steering);
			EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &AVehicle::Input_Steering);
		}
		if (BrakeAction)
		{
			// Bind Brake
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &AVehicle::Input_Brake);
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &AVehicle::Input_Brake);
		}
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
void AVehicle::CreateTires()
{
	AllTires.Add(FrontRightTire);
	AllTires.Add(FrontLeftTire);
	AllTires.Add(RearRightTire);
	AllTires.Add(RearLeftTire);

	//Add the tires
	for (int i = 0; i < AllTires.Num(); i++)
	{
		if (AllTires[i] != nullptr && SkeletalMesh)
		{	
			AllTires[i]->SetupAttachment(
				SkeletalMesh,
				FName(socketNames[i])
			);
			AllTires[i]->UpdateVehicleParameters(VehicleMass, WheelBaseLength, DistanceOfCentreOfGravityToTireAxis, CentreOfGravityHeight);
			//Store the socket name with wheel
			AllTires[i]->SocketName = socketNames[i];
		}
	}
}