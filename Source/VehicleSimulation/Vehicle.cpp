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
		{
			FrontRightTire->IsFrontTire = true;
			FrontRightTire->IsRightTire = true;
			FrontRightTire->TirePosition = FRONTRIGHT;
		}


		FrontLeftTire = CreateDefaultSubobject<UTire>(TEXT("Front-Left Tire"));
		if (FrontLeftTire)
		{
			FrontLeftTire->IsFrontTire = true;
			FrontLeftTire->TirePosition = FRONTLEFT;
		}


		//Create the back tires
		RearRightTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Right Tire"));
		if (RearRightTire)
		{
			RearRightTire->TirePosition = REARRIGHT;
			RearRightTire->IsRightTire = true;
		}

		RearLeftTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Left Tire"));
		if (RearLeftTire)
		{
			RearLeftTire->TirePosition = REARLEFT;
		}

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
void AVehicle::ApplyBraking(UTire* Tire, const float VehicleSpeedAtWheel, FVector WheelForward)
{
	if (Tire->IsGrounded && FMath::Abs(VehicleSpeedAtWheel) > 1.0f)
	{
		//Calculate the tire slip ratio

		float SlipRatio = Tire->CalculateSlipRatio(VehicleSpeedAtWheel);

		// Calculate braking force from slip ratio 

		// Limit Braking force  tire load and friction
		float MaxBrakingForce = Tire->TireLoad * Tire->GetFrictionCoefficient();
		float ActualBrakingForce = CurrentBrake * Tire->MagicFormula(MaxBrakingForce, SlipRatio);
		//Apply the anti dive mechcanics
		ActualBrakingForce *= AntiDivePercentage* FMath::Tan(FMath::DegreesToRadians(AntiDiveAngle));


		// Apply braking force at contact point in the opposite  direction to the velocity
		FVector BrakingForce = -WheelForward * ActualBrakingForce;

		ApplyLocationForce(BrakingForce, Tire->GetContactPoint());
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

		// Calculate lateral velocity at the centre of mass
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
	//Calculate acceleration with acceleration = change in velocity/change in time
	Acceleration = ((CurrentVelocity - LastVelocity) / DeltaTime);
	//Calculate the longitudinal and lateral acceleration
	float LongitudinalAcceleration = FVector::DotProduct(Acceleration, GetActorForwardVector());
	float LateralAcceleration = FVector::DotProduct(Acceleration, GetActorRightVector());
	//Store the current velocity of the next frame
	LastVelocity = CurrentVelocity;

	//Update the suspension
	CalculateSuspensionDynamics(DeltaTime, LongitudinalAcceleration, LateralAcceleration);

	//Calculate the drive force
	// Apply force each frame based on stored input
	if (MeshComponent)
	{


		CurrentVelocity = MeshComponent->GetPhysicsLinearVelocity();

		CalculateResistiveForce(CurrentVelocity);

		//Update the HUD parameters

		//Display the settings
		if (GEngine)
		{
			FVector XYVelocity = CurrentVelocity;
			XYVelocity.Z = 0;
			FVector XYAcceleration = Acceleration;
			XYAcceleration.Z = 0;
			GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Green, FString::Printf(TEXT("Speed %f km/h"), XYVelocity.Size() * 0.036));
			GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Green, FString::Printf(TEXT("Acceleration %f m/s^2"), XYAcceleration.Size() * 0.01));
			GEngine->AddOnScreenDebugMessage(3, 3.f, FColor::Green, FString::Printf(TEXT("Weight %f N"), VehicleMass * Gravity));
		}


		if (CurrentBrake > 0.0f )
		{
			IsBraking = true;
			AntiDiveFactor = (1.0f - AntiDivePercentage);
		}
		else
		{
			IsBraking = false;
			AntiDiveFactor = 1.0f;
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
				ApplyLocationForce(Tire->GetTraction(CurrentThrottle * ThrottleForce) * WheelForward, Tire->GetContactPoint());

				//Get lateral direction of the tire
				FVector WheelRight = FVector::CrossProduct(MeshComponent->GetUpVector(), WheelForward);
				//Get the wheel's velocity
				FVector VelocityAtWheel = MeshComponent->GetPhysicsLinearVelocityAtPoint(SocketLocation);

				//Handle braking
				float VehicleSpeedAtWheel = FVector::DotProduct(VelocityAtWheel, WheelForward);
				if (IsBraking)
				{
					ApplyBraking(Tire,VehicleSpeedAtWheel,WheelForward);
				}


				//Calculate the lateral and longitudinal speed
				float ForwardSpeed = FVector::DotProduct(VelocityAtWheel, WheelForward);
				float LateralSpeed = FVector::DotProduct(VelocityAtWheel, WheelRight);

				//Calculate the angle 
				float SlipAngle = Tire->CalculateSlipAngle(ForwardSpeed, LateralSpeed);
				//Calculate the lateral friction force

				float MaxGrip = Tire->GetLateralGrip();
				float DesiredForce = -Tire->MagicFormula(SlipAngle, MaxGrip);

				FVector LateralFriction = WheelRight * FMath::Clamp(DesiredForce, -MaxGrip, MaxGrip);

				ApplyLocationForce(LateralFriction, Tire->ContactPoint);


				float TireLoad = Tire->TireLoad;
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(4 + i, 3.f, FColor::Green, FString::Printf(TEXT("%s's tire load :%f N"), *Tire->GetName(), TireLoad));
			}
		}
	}
}
void AVehicle::SuspensionRayCast()
{
	//Vehicle up direction
	FVector VehicleUpDirection = MeshComponent->GetUpVector();

	// The ray points downwards relative to the vehicle's orientation

	FVector RayDirection = VehicleUpDirection * -1.0f;

	//Calculate the total force on acting on the suspension
	for (UTire*& Tire : AllTires)
	{

		//Get the Wheel and its corresponding ray
		const FWheelSuspensionSetting& Wheel = Tire->SuspensionSettings;
		FVector StartLocation = SkeletalMesh->GetSocketLocation(Tire->SocketName);

		FVector EndLocation = StartLocation + (RayDirection * (Wheel.SuspensionLength + Wheel.WheelRadius));

		FHitResult Hit;
		FCollisionQueryParams QueryParameters;
		QueryParameters.AddIgnoredActor(this);

		//Perform the Ray cast
		if (GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, TraceChannel, QueryParameters))
		{
			//Calculate compression using distance from mount point to the ground hit point
			float CurrentDistance = FVector::Dist(StartLocation, Hit.Location);

			//Get the compression of the wheel
			float Compression = Tire->GetCompression(CurrentDistance);

			//If we have a no compression then the tire is in air and has no load 
			if (Compression == 0)
			{
				Tire->IsGrounded = false;
				continue;
			}
			else
			{

				Tire->IsGrounded = true;
				//Update the number of grounded wheels for a given axis
			}

			// We need the velocity of the vehicle at the specific point where the suspension is attached
			FVector VelocityAtPoint = MeshComponent->GetPhysicsLinearVelocityAtPoint(StartLocation);

			// The damping force is based on the velocity along the suspension axis (Up Vector)
			float SuspensionVelocity = FVector::DotProduct(VelocityAtPoint, Tire->GetUpVector());

			//Get the suspension force of the tire
			float TireForceMagnitude = Tire->CalculateSuspensionForce(SuspensionVelocity);

			//Store the hit location
			Tire->StoreTireContactLocation(Hit.Location);
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
			AllTires[i]->UpdateVehicleParameters(VehicleMass, WheelBaseLength, TrackWidth, DistanceOfCentreOfGravityToFrontAxis,
				DistanceOfCentreOfGravityToRearAxis, CentreOfGravityHeight, Gravity);
			//
			const bool IsFrontTire = AllTires[i]->IsFrontTire;
			AllTires[i]->UpdateSuspension(IsFrontTire ? FrontSuspensionStiffness : RearSuspensionStiffness,
				IsFrontTire ? FrontSuspensionDamping : RearSuspensionDamping,
				SuspensionRestLength);
			//Store the socket name with wheel
			AllTires[i]->SocketName = socketNames[i];
		}
	}
}

void AVehicle::CalculateSuspensionDynamics(float DeltaTime, float LongitudinalAcceleration, const float LateralAcceleration)
{
	if (Initialized)
	{
		//Update the weight transfers 
		CalculatePitchWeightTransfer(LongitudinalAcceleration, LateralAcceleration);
		//Update the suspension compression using ray casts
		SuspensionRayCast();
		//Update pitch and heave
		CalculatePitchAndHeaveDynamics(DeltaTime, LongitudinalAcceleration);
		//Apply Force to the Physics Body to create torque/rotation naturally
		ApplySuspensionForceEffects();
	}
	else
	{
		Initialized = true;
	}
}

void AVehicle::CalculateResistiveForce(FVector Velocity)
{
	FVector TotalResistiveForce = FVector::ZeroVector;

	//Calculate drag using: Drag force = 0.5*drag Coefficient*Area*air density* speed^2
	float CurrentSpeed = FVector::DotProduct(Velocity, GetActorForwardVector());
	//Divide by 100  to convert from cm/s to m/s
	CurrentSpeed /= 100;
	float Area = Height * Width;
	//Convert from cm/s to m/s
	FVector DragForce = -Velocity.GetSafeNormal() * 0.5 * DragCoefficient * Area * AirDensity * CurrentSpeed * CurrentSpeed;
	//Store the drag
	TotalResistiveForce += DragForce;


	//Calculate the rolling resistance
	float TotalRollingResistance = 0.0f;
	for (UTire* Tire : AllTires)
	{
		if (Tire && Tire->IsGrounded)
		{
			TotalRollingResistance += Tire->GetRollingResistance();
		}
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

void AVehicle::CalculatePitchWeightTransfer(const float LongitudinalAcceleration, const float LateralAcceleration)
{
	//Calculate the static loads on the front and rear of the vehicle
	float VehicleWeight = VehicleMass * Gravity;

	float StaticFrontLoad = VehicleWeight * (DistanceOfCentreOfGravityToRearAxis / WheelBaseLength);
	float StaticRearLoad = VehicleWeight * (DistanceOfCentreOfGravityToFrontAxis / WheelBaseLength);

	// Dynamic weight transfer due to longitudinal acceleration
	// Weight transfer = (Mass × Acceleration × CG Height) / Wheelbase
	float LongitudinalWeightTransfer = (VehicleMass * LongitudinalAcceleration * CentreOfGravityHeight) / WheelBaseLength;
	// Multiple weight transfer by anti dive
	LongitudinalWeightTransfer *= AntiDiveFactor;
	float LateralForceOnVehicle = LateralAcceleration * VehicleMass;
	float LateralChangeInTireLoad = TrackWidth > 0 ? LateralForceOnVehicle * (CentreOfGravityHeight / TrackWidth) : 0;
	// During acceleration (positive), weight transfers to rear
	// During braking (negative), weight transfers to front
	float DynamicFrontLoad = StaticFrontLoad - LongitudinalWeightTransfer;
	float DynamicRearLoad = StaticRearLoad + LongitudinalWeightTransfer;

	// Add pitch-induced weight redistribution
	// Pitch affects load distribution: nose up = more rear load
	float PitchWeightTransfer = (VehicleMass * Gravity * PitchAngle * CentreOfGravityHeight) / WheelBaseLength;
	DynamicFrontLoad -= PitchWeightTransfer;
	DynamicRearLoad += PitchWeightTransfer;

	// Add suspension force contribution sand store final dynamic loads
	FrontDynamicLoad = FMath::Max(0.0f, DynamicFrontLoad);
	RearDynamicLoad = FMath::Max(0.0f, DynamicRearLoad);
	//Update the normal force on the tires
	for (UTire*& Tire : AllTires)
	{
		//Get the combination of the static and longitudinal weight transfer for the tire.
		float DynamicTireLoad = (Tire->IsFrontTire ? FrontDynamicLoad : RearDynamicLoad) / 2;
		//Add the lateral load
		DynamicTireLoad += LateralChangeInTireLoad * (Tire->IsRightTire ? 1 : -1) / 2;
		DynamicTireLoad = FMath::Max(0.0f, DynamicTireLoad);
		Tire->UpdateTireLoad(DynamicTireLoad);
	}
}

void AVehicle::CalculatePitchAndHeaveDynamics(float DeltaTime, float  LongitudinalAcceleration)
{
	FrontSuspensionForce = 0.0f;
	RearSuspensionForce = 0.0f;
	//Obtain the suspension forces from the wheels
	for (UTire*& Tire : AllTires)
	{
		if (Tire->IsGrounded)
		{
			//Handles grouping front spring forces
			if (Tire->IsFrontTire)
			{
				FrontSuspensionForce += Tire->GetSuspensionForce();
			}
			//Handles grouping rear spring forces
			else
			{
				RearSuspensionForce += Tire->GetSuspensionForce();
			}
		}
	}

	//Use the suspension forces to obtain the pitch moment
	float PitchMoment = (RearSuspensionForce * DistanceOfCentreOfGravityToRearAxis) - (FrontSuspensionForce * DistanceOfCentreOfGravityToFrontAxis);

	float WeightTransferMoment = VehicleMass * LongitudinalAcceleration * CentreOfGravityHeight;
	//Apply anti dive factor
	WeightTransferMoment *= AntiDiveFactor;
	PitchMoment += WeightTransferMoment;

	//Get the pitch acceleration from the moment
	float PitchAcceleration = PitchInertia != 0.0f ? PitchMoment / PitchInertia : 0;
	//Update the pitch velocity and angle
	PitchVelocity += PitchAcceleration * DeltaTime;
	PitchAngle += PitchVelocity * DeltaTime;

	// Add pitch damping to prevent oscillation
	PitchVelocity *= PitchDamping;

	// Clamp pitch angle to realistic limits
	PitchAngle = FMath::Clamp(PitchAngle, FMath::DegreesToRadians(-15.0f), FMath::DegreesToRadians(15.0f));

	//Get the overall vertical force on the 
	float TotalSuspensionForce = FrontSuspensionForce + RearSuspensionForce;
	float VehicleWeight = VehicleMass * Gravity;
	float NetVerticalForce = TotalSuspensionForce - VehicleMass * Gravity;
	//Calculate the heave acceleration with acceleration = force/mass
	float HeaveAcceleration = VehicleMass != 0 ? NetVerticalForce / VehicleMass : 0;
	// Update heave velocity and position
	HeaveVelocity += HeaveAcceleration * DeltaTime;
	HeavePosition += HeaveVelocity * DeltaTime;

	// Add heave damping
	HeaveDamping = 0.98f;
	HeaveVelocity *= HeaveDamping;

	// Clamp to realistic limits
	HeavePosition = FMath::Clamp(HeavePosition, -20.0f, 20.0f);
}

void AVehicle::ApplySuspensionForceEffects()
{

	FVector UpVector = MeshComponent->GetUpVector();
	for (UTire*& Tire : AllTires)
	{
		ApplyLocationForce(Tire->GetSuspensionForce() * UpVector, SkeletalMesh->GetSocketLocation(Tire->SocketName));
	}

}

void AVehicle::ApplyLocationForce(FVector Force, FVector Position)
{
	if (MeshComponent)
	{
		MeshComponent->AddForceAtLocation(Force, Position);
	}
}