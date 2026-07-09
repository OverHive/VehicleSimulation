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

		MeshComponent->SetAngularDamping(2.5f);

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
		RearRightTire->IsRightTire = true;

		RearLeftTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Left Tire"));
		RearLeftTire->IsRightTire = true;
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
	CurrentSteeringAngle = FMath::FInterpTo(CurrentSteeringAngle, CurrentSteering * MaxSteeringAngle, DeltaTime, SteeringInterpSpeed);
	if (FMath::Abs(CurrentSteering) < SteeringReleaseThreshold)
	{
		FVector AngularVelocity = MeshComponent->GetPhysicsAngularVelocityInRadians();
		float YawAngularVelocity = AngularVelocity.Z;

		// Calculate lateral velocity at the center of mass
		FVector LinearVelocity = MeshComponent->GetPhysicsLinearVelocity();
		FVector RightVector = GetActorRightVector();
		float LateralVelocity = FVector::DotProduct(LinearVelocity, RightVector);

		// Progressive damping: more damping at higher speeds, less at low speeds
		float SpeedFactor = FMath::Clamp(FMath::Abs(LinearVelocity.Size()) / 1000.0f, 0.1f, 1.0f);

		// Base damping on vehicle mass and speed
		float DampingCoefficient = AngularDampingWhenSteeringReleased * VehicleMass * SpeedFactor * 10.0f;

		// Apply counter-torque to angular velocity
		FVector CounterTorque = FVector(0.0f, 0.0f, -YawAngularVelocity * DampingCoefficient);
		MeshComponent->AddTorqueInRadians(CounterTorque);

		// Additional: Apply lateral force to help straighten the vehicle
		float LateralCorrectionForce = -LateralVelocity * DampingCoefficient * 0.5f;
		MeshComponent->AddForce(RightVector * LateralCorrectionForce);
	}

	//Update the suspension
	SuspensionRayCast();
	//Calculate the drive force
	// Apply force each frame based on stored input
	if (MeshComponent)
	{


		CurrentVelocity = MeshComponent->GetPhysicsLinearVelocity();

		CalculateResistiveForce(CurrentVelocity);
		// Braking
		if (CurrentBrake > 0.0f)
		{
			FVector BrakingForce = -MeshComponent->GetPhysicsLinearVelocity().GetSafeNormal() * (CurrentBrake * BrakeForce);
			MeshComponent->AddForce(BrakingForce, NAME_None, false);
		}
		//Update the HUD parameters

		
		//Calculate acceleration with acceleration = change in velocity/change in time
		FVector Acceleration = ((CurrentVelocity - LastVelocity) / DeltaTime);
		//Calculate the longitudinal and lateral acceleration
		float LongitudinalAcceleration = FVector::DotProduct(Acceleration, GetActorForwardVector());
		float LateralAcceleration = FVector::DotProduct(Acceleration, GetActorRightVector());
		//Store the current velocity of the next frame
		LastVelocity = CurrentVelocity;
		//Display the settings
		if (GEngine)
		{
			FVector XYVelocity = CurrentVelocity;
			XYVelocity.Z = 0;
			FVector XYAcceleration = Acceleration;
			XYAcceleration.Z = 0;
			GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Green, FString::Printf(TEXT("Speed %f km/h"), XYVelocity.Size() * 0.036));
			GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Green, FString::Printf(TEXT("Acceleration %f m/s^2"), XYAcceleration.Size() * 0.01));
		}

		for (int i = 0; i < AllTires.Num(); i++)
		{
			UTire* Tire = AllTires[i];
			FVector SocketLocation = SkeletalMesh->GetSocketLocation(Tire->SocketName);
			//If we have a vaild tire
			if (Tire)
			{
				//Update the steering of the wheel
				Tire->UpdateSteering(CurrentSteeringAngle);
				//Get the direction of the wheel
				FVector WheelForward = Tire->GetForwardVector();
				// Update and apply the traction from the wheels
				Tire->UpdateTireLoad(LongitudinalAcceleration, LateralAcceleration);
				MeshComponent->AddForceAtLocation(Tire->GetTraction(CurrentThrottle * ThrottleForce) * WheelForward, SocketLocation);
	
				//Get 
				FVector WheelRight = FVector::CrossProduct(MeshComponent->GetUpVector(), WheelForward);
				//Get the wheel's velocity
				FVector VelocityAtWheel = MeshComponent->GetPhysicsLinearVelocityAtPoint(SocketLocation);
				
				//Calculate the lateral speed
				float LateralSpeed = FVector::DotProduct(VelocityAtWheel, WheelRight);


				// Force opposing sideways slip, capped by the tire's grip
				// replace with Lateral Force calculated with the Magic Formula in future
				float LateralStiffness = 5.0f;
				float DesiredForce = -LateralSpeed * LateralStiffness;

				float MaxGrip = Tire->GetLateralGrip();
				FVector LateralFriction = WheelRight * FMath::Clamp(DesiredForce, -MaxGrip, MaxGrip);

				MeshComponent->AddForceAtLocation(LateralFriction, Tire->ContactPoint);


				float TireLoad = Tire->TireLoad;
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(3 + i, 3.f, FColor::Green, FString::Printf(TEXT("%s's tire load :%f N"), *Tire->GetName(), TireLoad));
			}
		}
	}
}
void AVehicle::SuspensionRayCast()
{
	for (UTire*& Tire : AllTires)
	{

		//Get the Wheel and its corresponding ray
		const FWheelSuspensionSetting& Wheel = Tire->SuspensionSettings;
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

			//Calculate the spring compression using the difference between the suspension length and the bottom of the wheel
			float Compression = Wheel.SuspensionLength - (CurrentDistance - Wheel.WheelRadius);


			// Clamp compression to prevent negative values/extreme forces
			Compression = FMath::Max(0.0f, Compression);
			//If we have a no compression then the tire is in air and has no load 
			if (Compression == 0)
			{
				Tire->IsGrounded = false;
				continue;
			}
			else
			{
				Tire->IsGrounded = true;
			}


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

			//Apply Force to the Physics Body to create torque/rotation naturally
			MeshComponent->AddForceAtLocation(TotalForceVector, Hit.Location);
			//Update the wheel's suspension
			Tire->UpdateWheelSuspension(SpringForceMagnitude* VehicleUpDirection, Hit.Location);
		}
		else
		{
			Tire->IsGrounded = false;
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
			AllTires[i]->UpdateFrictionCoefficient(1.0);
			AllTires[i]->UpdateVehicleParameters(VehicleMass, WheelBaseLength,TrackWidth, DistanceOfCentreOfGravityToFrontAxis, DistanceOfCentreOfGravityToRearAxis, CentreOfGravityHeight);
			//Store the socket name with wheel
			AllTires[i]->SocketName = socketNames[i];
		}
	}
}

void AVehicle::CalculateResistiveForce(FVector Velocity)
{
	FVector TotalResistiveForce = FVector::ZeroVector;

	//Calculate drag using: Drag force = 0.5*drag Coefficient*Area*air density* speed^2
	float CurrentSpeed = FVector::DotProduct(Velocity, GetActorForwardVector());
	float Area = Height * Width;
	//Convert from cm/s to m/s
	FVector DragForce = -Velocity.GetSafeNormal() * 0.005 * DragCoefficient * Area * AirDensity * CurrentSpeed * CurrentSpeed;
	//Store the drag
	TotalResistiveForce += DragForce;


	//Calculate the rolling resistance
	float TotalRollingResistance = 0.0f;
	for (UTire* Tire : AllTires)
	{
		if (Tire && Tire->IsGrounded)

			TotalRollingResistance += Tire->GetRollingResistance();
	}
	//Only apply rolling resistance if the vehicle is moving
	if (Velocity.Size() > 0.1f)
	{
		FVector RollingResistanceForce = -Velocity.GetSafeNormal() * TotalRollingResistance;
		TotalResistiveForce += RollingResistanceForce;
	}
	//Subtract resistive forces from the diving force
	MeshComponent->AddForce(TotalResistiveForce, NAME_None, false);

}
