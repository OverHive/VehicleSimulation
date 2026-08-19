// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h" 

// Sets default values
AVehicle::AVehicle()
{
	TotalVehicleMass = VehicleSprungMass + (RearUnsprungMass + FrontUnsprungMass) * 2;
	//Create the presets
	VehicleSettings = VehiclePresets();
	//Initialise the tire loads
	UpdateStaticLoads();
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//Create the Root Component
	PhysicMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhysicMesh"));
	//For attaching wheels
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleComponent"));

	if (SkeletalMesh && PhysicMesh)
	{
		RootComponent = PhysicMesh;
		SkeletalMesh->SetupAttachment(PhysicMesh);

		// Enable physic and gravity;
		PhysicMesh->SetSimulatePhysics(true);
		PhysicMesh->SetEnableGravity(true);
		PhysicMesh->SetMassOverrideInKg(NAME_None, TotalVehicleMass, true);

		//For Displaying visual effect
		VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
		VisualMesh->SetupAttachment(PhysicMesh);
		VisualMesh->SetSimulatePhysics(false);

		//Get the original position of the Visual mesh
		DefaultVisualMeshPosition = VisualMesh->GetRelativeLocation();

		//Create the front tires
		FrontRightTire = CreateDefaultSubobject<UTire>(TEXT("Front-Right Tire"));
		if (FrontRightTire)
		{
			FrontRightTire->IsFrontTire = true;
			FrontRightTire->IsRightTire = true;
		}

		FrontLeftTire = CreateDefaultSubobject<UTire>(TEXT("Front-Left Tire"));
		if (FrontLeftTire)
		{
			FrontLeftTire->IsFrontTire = true;
		}

		//Create the rears tires
		RearRightTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Right Tire"));
		if (RearRightTire)
		{
			RearRightTire->IsRightTire = true;
		}

		RearLeftTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Left Tire"));

		CreateTires();
	}
}

// Called when the game starts or when spawned
void AVehicle::BeginPlay()
{
	PhysicMesh->SetMassOverrideInKg(NAME_None, TotalVehicleMass, true);
	UpdateStaticLoads();
	SetFromPreset(0);
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
void AVehicle::ApplyBraking(UTire* Tire, const float VehicleSpeedAtWheel, float DeltaTime)
{
	if (Tire->IsGrounded && FMath::Abs(VehicleSpeedAtWheel) > 20.0f)
	{
		// Apply braking force at contact point in the opposite  direction to the velocity

		float BrakingForce = GetTireBrakingForce(Tire, DeltaTime);
		if (!IsLongitudinalControlled)
		{
			BrakingForce = Tire->FrictionCircle(BrakingForce, Tire->GetLateralGrip(), IsLongitudinalControlled);
		}
		Tire->UpdateLongitudinalForce(BrakingForce);

		ApplyWheelForce(Tire, BrakingForce, -FMath::Sign(VehicleSpeedAtWheel) * Tire->GetForwardVector(), !Tire->IsFrontTire);
	}
}
// Called every frame
void AVehicle::Tick(float DeltaTime)
{
	GEngine->AddOnScreenDebugMessage(17, 3.f, FColor::Purple, FString::Printf(TEXT("Debug mode:%f N"), DebugSetting));

	if (DebugSetting != 0)
	{
		FakeAcceleration += DebugSetting > 0 ? 1 : -1;
		CurrentBrakingForce += DebugSetting * 10000;
		ThrottleForce += DebugSetting * 10000;
		DebugSetting = 0;
	}

	Super::Tick(DeltaTime);
	CurrentSteeringAngle = FMath::FInterpTo(CurrentSteeringAngle, CurrentSteering * MaxSteeringAngle, DeltaTime, SteeringInterpSpeed);

	bool IsThrottleActive = FMath::Abs(CurrentThrottle) > 0.08f;
	bool IsSteeringActive = FMath::Abs(CurrentSteering) > 0.08f;
	//Select the dominating force based on the state of the throttle and steering
	if (IsThrottleActive && !IsSteeringActive)
	{
		IsLongitudinalControlled = true;  // Pure longitudinal motion
	}
	else if (IsSteeringActive && !IsThrottleActive)
	{
		IsLongitudinalControlled = false; // Lateral/cornering dominated
	}

	if (PhysicMesh)
	{
		CalculateResistiveForces(CurrentVelocity, DeltaTime);

		//Toggle brakes base on the current braking input 
		IsBraking = CurrentBrake > 0.0f;

		CurrentDrivingForce = 0.0f;
		//Apply traction through the front tires
		for (UTire* Tire : FrontTires)
		{
			if (Tire->IsFrontTire)
			{
				float TireTraction = GetTireTraction(Tire);
				if (!IsLongitudinalControlled)
				{
					TireTraction = Tire->FrictionCircle(TireTraction, Tire->GetLateralGrip(), IsLongitudinalControlled);
				}
				ApplyWheelForce(Tire, TireTraction, Tire->GetForwardVector(), Tire->IsFrontTire);

				Tire->UpdateLongitudinalForce(TireTraction);
				CurrentDrivingForce += TireTraction;
			}
		}

		for (int i = 0; i < AllTires.Num(); i++)
		{
			UTire* Tire = AllTires[i];
			//If we have a vaild tire
			if (Tire)
			{
				FVector WheelForward = Tire->GetForwardVector();
				FVector VelocityAtWheel = PhysicMesh->GetPhysicsLinearVelocityAtPoint(SkeletalMesh->GetSocketLocation(Tire->SocketName));
				float SpeedAtWheel = FVector::DotProduct(VelocityAtWheel, WheelForward);

				//Handle braking
				if (IsBraking)
				{
					ApplyBraking(Tire, SpeedAtWheel, DeltaTime);
				}
				//Update the wheel's parameters 
				UpdateWheel(Tire, DeltaTime);

				//Calculate the lateral friction force
				float DesiredForce = Tire->GetLateralGrip();
				//Apply friction circle for longitudinal dominated motion
				if (IsLongitudinalControlled)
				{
					DesiredForce = Tire->FrictionCircle(Tire->GetCurrentLongitudinalForceOnTire(), DesiredForce, IsLongitudinalControlled);
				};

				FVector WheelRight = FVector::CrossProduct(PhysicMesh->GetUpVector(), WheelForward);
				FVector LateralFriction = WheelRight * -DesiredForce;

				ApplyLocationForce(LateralFriction, Tire->GetContactPoint());

				float TireLoad = Tire->TireLoad;
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(4 + i, 3.f, FColor::Green, FString::Printf(TEXT("%s's tire load :%f N"), *Tire->GetName(), TireLoad));
				Tire->ResetForces();
			}
		}
	}

	//Store the current velocity of the last frame
	LastVelocity = !CurrentVelocity.IsNearlyZero() ? CurrentVelocity : FVector::ZeroVector;

	//Get the current velocity
	CurrentVelocity = PhysicMesh->GetPhysicsLinearVelocity();

	//Calculate acceleration with acceleration = change in velocity/change in time
	Acceleration = ((CurrentVelocity - LastVelocity) / DeltaTime);
	//Calculate the longitudinal and lateral acceleration
	float LongitudinalAcceleration = FVector::DotProduct(Acceleration, GetActorForwardVector());
	float LateralAcceleration = FVector::DotProduct(Acceleration, GetActorRightVector());

	//Update the suspension
	CalculateSuspensionDynamics(DeltaTime, LongitudinalAcceleration, LateralAcceleration);

	float ForwardVelocity = FMath::Abs(FVector::DotProduct(CurrentVelocity, PhysicMesh->GetForwardVector() * 0.036));
	if (GEngine)
	{
		float ForwardAcceleration = FMath::Abs(FVector::DotProduct(Acceleration, PhysicMesh->GetForwardVector() * 0.01));
		GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Green, FString::Printf(TEXT("Speed %f km/h"), ForwardVelocity));
		GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Green, FString::Printf(TEXT("Acceleration %f m/s^2"), ForwardAcceleration));
		GEngine->AddOnScreenDebugMessage(3, 3.f, FColor::Green, FString::Printf(TEXT("Weight %f N"), VehicleSprungMass * Gravity));
		float ForwardVelocityInMetres = (ForwardVelocity / 0.036) / 100;
		if (LastFakeAcceleration != FakeAcceleration)
		{
			float LateralForceOnVehicle = FakeAcceleration * VehicleSprungMass;
			float LateralChangeInTireLoad = TrackWidth > 0 ? LateralForceOnVehicle * (CentreOfGravityHeight / TrackWidth) : 0;
			FText TestMessage = FText::Format(
				NSLOCTEXT("Name space", "CurrentDrag", "Acceleration : {1} m/s^2, Lateral Weight transfer: {0} Kg"),
				FText::AsNumber(LateralChangeInTireLoad), FText::AsNumber(FakeAcceleration));
			FMessageLog MessageLog(FName("testing"));
			MessageLog.Info(TestMessage);
			LastFakeAcceleration = FakeAcceleration;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         .0;
		}
	}

}
void AVehicle::SuspensionRayCast()
{
	//Vehicle up direction
	FVector VehicleUpDirection = PhysicMesh->GetUpVector();

	// The ray points downwards relative to the vehicle's orientation
	FVector RayDirection = VehicleUpDirection * -1.0f;
	int i = 0;
	//Calculate the forces on acting on the suspension
	for (UTire*& Tire : AllTires)
	{
		//Get the Wheel and its corresponding ray
		const FWheelSuspensionSetting& Wheel = Tire->SuspensionSettings;
		FVector StartLocation = SkeletalMesh->GetSocketLocation(Tire->SocketName);
		StartLocation.Z += 2.0f;
		FVector EndLocation = StartLocation + (RayDirection * (Wheel.SuspensionLength + 1));

		FHitResult Hit;
		FCollisionQueryParams QueryParameters;
		QueryParameters.AddIgnoredActor(this);

		//Perform the Ray cast
		if (GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, TraceChannel, QueryParameters))
		{
			//Calculate compression using distance from mount point to the ground hit point
			float CurrentDistance = RoundToDecimalPoint(FVector::Dist(StartLocation, Hit.Location) - 1.0);
			CurrentDistance = FMath::Min(CurrentDistance, Wheel.SuspensionLength);

			//Get the compression of the wheel
			float Compression = Tire->GetCompression(CurrentDistance);
			Tire->IsGrounded = true;

			// We need the velocity of the vehicle at the specific point where the suspension is attached
			FVector VelocityAtPoint = PhysicMesh->GetPhysicsLinearVelocityAtPoint(StartLocation);

			// The damping force is based on the velocity along the suspension axis (Up Vector)
			float SuspensionVelocity = FVector::DotProduct(VelocityAtPoint, Tire->GetUpVector());
			//Update the suspension force of the tire
			float S = Tire->CalculateSuspensionForce(SuspensionVelocity);
			GEngine->AddOnScreenDebugMessage(22 + i, 3.f, FColor::Red, FString::Printf(TEXT("%s's tire load :%f N"), *Tire->GetName(), Tire->TireLoad));
			//Store the hit location and normal
			Tire->StoreTireContactInformation(Hit);

			Tire->UpdateRollingRadius(StartLocation);
			i++;
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
		if (DebugAction)
		{
			// Bind Debug
			EnhancedInputComponent->BindAction(DebugAction, ETriggerEvent::Triggered, this, &AVehicle::Input_Debug);
			EnhancedInputComponent->BindAction(DebugAction, ETriggerEvent::Completed, this, &AVehicle::Input_Debug);
		}
		if (DebugAction)
		{
			// Bind Gear change
			EnhancedInputComponent->BindAction(GearAction, ETriggerEvent::Triggered, this, &AVehicle::Input_Gear);
			EnhancedInputComponent->BindAction(GearAction, ETriggerEvent::Completed, this, &AVehicle::Input_Gear);
		}
	}

}

void AVehicle::SetFromPreset(const float Index)
{
	//Get the preset
	CarSettings Preset = VehicleSettings.GetPreset(Index);
	VehiclePresetName = Preset.VehicleName;
	VehicleSprungMass = Preset.VehicleSprungMass;
	Height = Preset.Height;
	Width = Preset.Width;
	DragCoefficient = Preset.DragCoefficient;
	WheelBaseLength = Preset.WheelBaseLength;
	CentreOfGravityHeight = Preset.CentreOfGravityHeight;
	DistanceOfCentreOfGravityToFrontAxis = Preset.DistanceOfCentreOfGravityToFrontAxis;


	DistanceOfCentreOfGravityToRearAxis = Preset.DistanceOfCentreOfGravityToRearAxis;
	TrackWidth = Preset.TrackWidth;
	GearRatios = Preset.GearRatios;
	FinalDriveRatio = Preset.FinalDriveRatio;
	DrivetrainEfficiency = Preset.DrivetrainEfficiency;
	PitchInertia = Preset.PitchInertia;
	PitchStiffness = Preset.PitchStiffness;
	PitchDamping = Preset.PitchDamping;

	FrontSuspensionStiffness = Preset.FrontSuspensionStiffness;
	RearSuspensionStiffness = Preset.RearSuspensionStiffness;
	FrontSuspensionDamping = Preset.FrontSuspensionDamping;
	RearSuspensionDamping = Preset.RearSuspensionDamping;
	FrontUnSprungDamping = Preset.FrontUnSprungDamping;
	RearUnSprungDamping = Preset.RearUnSprungDamping;
	FrontUnsprungMass = Preset.FrontUnsprungMass;
	RearUnsprungMass = Preset.RearUnsprungMass;
	FrontTireVerticalStiffness = Preset.FrontTireVerticalStiffness;
	RearTireVerticalStiffness = Preset.RearTireVerticalStiffness;
	FrontWheelRadius = Preset.FrontWheelRadius;
	RearWheelRadius = Preset.RearWheelRadius;
	FrontTireFriction = Preset.FrontTireFriction;
	RearTireFriction = Preset.RearTireFriction;
	HeaveDamping = Preset.HeaveDamping;
	TorqueCurve = Preset.TorqueCurve;
	CurveStep = Preset.CurveStep;
	//Recalculate the loads and updates the tires
	TotalVehicleMass = VehicleSprungMass + (RearUnsprungMass + FrontUnsprungMass) * 2;
	PhysicMesh->SetMassOverrideInKg(NAME_None, TotalVehicleMass, true);
	UpdateStaticLoads();
	CreateTires();
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
void AVehicle::Input_Debug(const FInputActionValue& Value)
{
	DebugSetting = Value.Get<float>();
}
void AVehicle::Input_Gear(const FInputActionValue& Value)
{
	float Switch = Value.Get<float>();
	if (Switch > 0 && GearIndex + 1 < GearRatios.Num())
	{
		GearIndex++;
	}
	else if (Switch < 0 && GearIndex - 1 >= 0)
	{
		GearIndex--;
	}
	GearRatio = GearRatios[GearIndex];
}

void AVehicle::CreateTires()
{
	AllTires.Empty();
	FrontTires.Empty();

	AllTires.Add(FrontRightTire);
	AllTires.Add(FrontLeftTire);
	AllTires.Add(RearRightTire);
	AllTires.Add(RearLeftTire);

	FrontTires.Add(FrontRightTire);
	FrontTires.Add(FrontLeftTire);
	//Add the tires
	for (int i = 0; i < AllTires.Num(); i++)
	{
		UTire* Tire = AllTires[i];
		if (Tire != nullptr && SkeletalMesh)
		{
			Tire->SetupAttachment(
				SkeletalMesh,
				FName(socketNames[i])
			);
			//Setup the suspension
			const bool IsFrontTire = Tire->IsFrontTire;
			//Update the suspension
			Tire->UpdateSuspension(IsFrontTire ? FrontSuspensionStiffness : RearSuspensionStiffness,
				IsFrontTire ? FrontSuspensionDamping : RearSuspensionDamping,
				MaxSuspensionLength,
				IsFrontTire ? FrontTireVerticalStiffness : RearTireVerticalStiffness,
				IsFrontTire ? FrontUnsprungMass : RearUnsprungMass, (IsFrontTire ? StaticFrontLoad : StaticRearLoad) / 2);
			//Update the radius and friction of the wheel based on position
			Tire->UpdateTireRadius(IsFrontTire ? FrontWheelRadius : RearWheelRadius);
			Tire->UpdateFrictionCoefficient(IsFrontTire ? FrontTireFriction : RearTireFriction);
			//Store the socket name with wheel
			Tire->SocketName = socketNames[i];
		}
	}
}

void AVehicle::CalculateSuspensionDynamics(float DeltaTime,
	float LongitudinalAcceleration, const float LateralAcceleration)
{

	//Update the suspension compression using ray casts
	SuspensionRayCast();
	//Update unsprung masses
	CalculateUnsprungMassDynamics(DeltaTime);
	//Update pitch and heave
	CalculatePitchAndHeaveDynamics(DeltaTime, LongitudinalAcceleration);
	//Update the weight transfers 
	CalculatePitchWeightTransfer(LongitudinalAcceleration, LateralAcceleration);
	//Apply Force to the physics body to create visual suspension movement
	ApplySuspensionForceEffects();
}

void AVehicle::CalculateResistiveForces(FVector Velocity, float DeltaTime)
{
	FVector MovementDirection = -Velocity.GetSafeNormal();
	//Calculate drag using: Drag force = 0.5*drag Coefficient*Area*air density* speed^2
	float CurrentSpeed = Velocity.Size() / 100.0f;
	float Area = Height * Width / 10000;
	//Calculate drag in Newtons
	FVector DragForce = MovementDirection * 0.5 * DragCoefficient * Area * AirDensity * CurrentSpeed * CurrentSpeed;
	// Convert Newtons to Unreal force units (kg·cm/s²)
	DragForce *= 100;
	//Apply drag
	PhysicMesh->AddForce(DragForce, NAME_None, false);
	CurrentDrag = 0.0f;
	GEngine->AddOnScreenDebugMessage(26, 3.f, FColor::Green, FString::Printf(TEXT("Drag %f N"), DragForce.Size()));
	CurrentDrag = DragForce.Size();

	//Calculate the rolling resistance
	for (UTire* Tire : AllTires)
	{
		if (Tire && Tire->IsGrounded)
		{
			float TireRollingResistance = GetTireRollingResistance(Tire, DeltaTime);
			ApplyWheelForce(Tire, TireRollingResistance, MovementDirection, Tire->IsFrontTire);
		}
	}
}

void AVehicle::CalculatePitchWeightTransfer(const float LongitudinalAcceleration, const float LateralAcceleration)
{
	// Dynamic weight transfer due to longitudinal acceleration
	// Weight transfer = (Mass × Acceleration × CG Height) / Wheelbase
	float LongitudinalWeightTransfer = (VehicleSprungMass * LongitudinalAcceleration * CentreOfGravityHeight) / WheelBaseLength;
	float LateralForceOnVehicle = LateralAcceleration * VehicleSprungMass;
	float LateralChangeInTireLoad = TrackWidth > 0 ? LateralForceOnVehicle * (CentreOfGravityHeight / TrackWidth) : 0;
	// During acceleration (positive), weight transfers to rear
	// During braking (negative), weight transfers to front
	float DynamicFrontLoad = StaticFrontLoad - LongitudinalWeightTransfer;
	float DynamicRearLoad = StaticRearLoad + LongitudinalWeightTransfer;

	// Add pitch-induced weight redistribution
	// Pitch affects load distribution: nose up = more rear load
	float PitchWeightTransfer = (VehicleSprungMass * Gravity * sin(PitchAngle) * CentreOfGravityHeight) / WheelBaseLength;
	DynamicFrontLoad -= PitchWeightTransfer;
	DynamicRearLoad += PitchWeightTransfer;

	// Add suspension force contribution and store final dynamic loads
	FrontDynamicLoad = FMath::Max(0.0f, DynamicFrontLoad);
	RearDynamicLoad = FMath::Max(0.0f, DynamicRearLoad);
	//Update the normal force on the tires
	for (UTire*& Tire : AllTires)
	{
		//Get the combination of the static and longitudinal weight transfer for the tire.
		float DynamicTireLoad = (Tire->IsFrontTire ? FrontDynamicLoad : RearDynamicLoad) / 2;
		//Add the lateral load
		DynamicTireLoad += LateralChangeInTireLoad * (Tire->IsRightTire ? -1 : 1) / 2;
		DynamicTireLoad = FMath::Max(0.0f, DynamicTireLoad);
		Tire->UpdateTireLoad(DynamicTireLoad);
	}
}

void AVehicle::CalculateUnsprungMassDynamics(float DeltaTime)
{
	//Reset the unspring forces
	FrontUnsprungForce = 0.0f;
	RearUnsprungForce = 0.0f;

	for (UTire*& Tire : AllTires)
	{
		if (Tire->IsGrounded)
		{
			//Select a unsprung mass based on tire position 
			float UnsprungMass = Tire->IsFrontTire ? FrontUnsprungMass : RearUnsprungMass;

			// Get suspension force
			float SuspensionForce = Tire->GetSuspensionForce();

			// Net force on unsprung mass = Tire force (tire load) - Suspension force
			float NetUnsprungForce = Tire->TireLoad - SuspensionForce;

			// Update unsprung mass dynamics
			float UnsprungAcceleration = UnsprungMass != 0 ? NetUnsprungForce / UnsprungMass : 0;
			//Obtain the suspension forces from the wheels and update the unsprung mass positions
			if (Tire->IsFrontTire)
			{
				FrontUnsprungVelocity += UnsprungAcceleration * DeltaTime;
				FrontUnsprungPosition += FrontUnsprungVelocity * DeltaTime;
				FrontUnsprungForce += SuspensionForce; // Store for sprung mass calculation
			}
			else
			{
				RearUnsprungVelocity += UnsprungAcceleration * DeltaTime;
				RearUnsprungPosition += RearUnsprungVelocity * DeltaTime;
				RearUnsprungForce += SuspensionForce;
			}

			// Add damping to unsprung mass
			if (Tire->IsFrontTire)
				FrontUnsprungVelocity *= FMath::Exp(-FrontUnSprungDamping * DeltaTime);
			else
				RearUnsprungVelocity *= FMath::Exp(-RearUnSprungDamping * DeltaTime);
		}
	}
}

void AVehicle::CalculatePitchAndHeaveDynamics(float DeltaTime, float  LongitudinalAcceleration)
{
	//Use the suspension forces to obtain the pitch moment
	float PitchMoment = (FrontUnsprungForce * DistanceOfCentreOfGravityToFrontAxis) - (RearUnsprungForce * DistanceOfCentreOfGravityToRearAxis);


	float WeightTransferMoment = VehicleSprungMass * LongitudinalAcceleration * CentreOfGravityHeight;
	PitchMoment += WeightTransferMoment;

	float PitchRestoringMoment = -(PitchStiffness + VehicleSprungMass * Gravity * CentreOfGravityHeight) * FMath::Sin(PitchAngle);
	GEngine->AddOnScreenDebugMessage(11, 3.f, FColor::Green, FString::Printf(TEXT("Restoring: %i"), abs(PitchRestoringMoment) > abs(PitchMoment)));
	PitchMoment += PitchRestoringMoment;
	//Get the pitch acceleration from the moment
	float PitchAcceleration = PitchInertia != 0.0f ? PitchMoment / PitchInertia : 0;
	//Update the pitch velocity and angle
	PitchVelocity += PitchAcceleration * DeltaTime;

	// Add pitch damping to prevent oscillation
	PitchVelocity *= FMath::Exp(-PitchDamping * DeltaTime); ;
	PitchAngle += PitchVelocity * DeltaTime;
	GEngine->AddOnScreenDebugMessage(10, 3.f, FColor::Green, FString::Printf(TEXT("Pitch: %f N"), PitchAngle));

	// Clamp pitch angle to realistic limits
	PitchAngle = FMath::Clamp(PitchAngle, FMath::DegreesToRadians(-15.0f), FMath::DegreesToRadians(15.0f));
	//Get the overall vertical force on the suspension
	float TotalSuspensionForce = FrontUnsprungForce + RearUnsprungForce;

	float NetVerticalForce = TotalSuspensionForce - VehicleWeight;

	// Add restoring force to bring heave back to equilibrium
	float HeaveRestoringForce = (FrontSuspensionStiffness + RearSuspensionStiffness) * HeavePosition;
	NetVerticalForce -= HeaveRestoringForce;

	//Calculate the heave acceleration with acceleration = force/mass
	float HeaveAcceleration = VehicleSprungMass != 0 ? NetVerticalForce / VehicleSprungMass : 0;
	// Update heave velocity and position
	HeaveVelocity += HeaveAcceleration * DeltaTime;
	// Add heave damping
	HeaveVelocity *= FMath::Exp(-HeaveDamping * DeltaTime); ;
	HeavePosition += HeaveVelocity * DeltaTime;

	// Clamp to realistic limits
	HeavePosition = FMath::Clamp(HeavePosition, -10.0f, 10.0f);
}

void AVehicle::UpdateWheel(UTire* Tire, float DeltaTime)
{
	FVector SocketLocation = SkeletalMesh->GetSocketLocation(Tire->SocketName);
	//Get the lateral and longitudinal directions of the wheel
	FVector WheelForward = Tire->GetForwardVector();
	FVector WheelRight = FVector::CrossProduct(PhysicMesh->GetUpVector(), WheelForward);
	//Get the wheel's velocity
	FVector VelocityAtWheel = PhysicMesh->GetPhysicsLinearVelocityAtPoint(SocketLocation);
	//Update the steering of the wheel and its rotational velocity
	Tire->UpdateSteering(CurrentSteeringAngle);

	//Calculate the braking torque
	float EffectiveWheelRadius = Tire->GetRollingRadius();
	float BrakingTorque = EffectiveWheelRadius != 0 ? GetTireBrakingForce(Tire, DeltaTime) / EffectiveWheelRadius : 0;
	//Calculate the diving torque
	float Denominator = GearRatio * FinalDriveRatio * DrivetrainEfficiency;
	float DriveTorque = EffectiveWheelRadius != 0 && Tire->IsFrontTire ? GetTireTraction(Tire) * EffectiveWheelRadius : 0;
	//Calculate the torque from the resistive forces

	float ResistiveTorque = EffectiveWheelRadius != 0 ? (GetTireRollingResistance(Tire, DeltaTime) + CurrentDrag) / EffectiveWheelRadius : 0;
	//Get the net torque
	float NetTorque = DriveTorque - BrakingTorque - ResistiveTorque;

	//Use the torque to update the velocity of the wheel 
	Tire->UpdateWheelRotationalVelocity(NetTorque, DeltaTime);

	//Calculate the lateral and longitudinal speed of the wheel
	float  LongitudinalVelocity = FVector::DotProduct(VelocityAtWheel, WheelForward);
	float LateralVelocity = FVector::DotProduct(VelocityAtWheel, WheelRight);

	//Update the slip angle and ratio 
	Tire->UpdateSlipAngle(LongitudinalVelocity, LateralVelocity);
	Tire->UpdateSlipRatio(LongitudinalVelocity, IsBraking);
}

void AVehicle::ApplySuspensionForceEffects()
{
	if (VisualMesh)
	{
		// Convert pitch angle (radians) to a rotator
		FRotator PitchRotation(FMath::RadiansToDegrees(PitchAngle), 0.0f, 0.0f);

		// Apply the pitch
		VisualMesh->SetRelativeRotation(PitchRotation);

		// Apply heave (vertical displacement) along the local up vector
		FVector HeaveOffset = VisualMesh->GetUpVector() * HeavePosition;

		// Apply heave offset
		VisualMesh->SetRelativeLocation(DefaultVisualMeshPosition + HeaveOffset);
	}

}

void AVehicle::ApplyWheelForce(UTire* Tire, float ForceMagnitude, FVector Direction, bool IsForward)
{
	if (abs(ForceMagnitude) > 0)
	{
		ApplyLocationForce(ForceMagnitude * Direction, Tire->GetContactPoint());
		// Calculate moment arm to front wheels
		float FrontMomentArm = FVector::Distance(PhysicMesh->GetCenterOfMass(), Tire->GetContactPoint());

		// Calculate the counteracting torque needed
		float PitchMoment = ForceMagnitude * FrontMomentArm;
		PitchMoment *= IsForward ? 1 : -1;
		// Apply counter-torque
		FVector PitchCounterTorque = GetActorRightVector() * (PitchMoment);
		PhysicMesh->AddTorqueInRadians(PitchCounterTorque);
	}
}

void AVehicle::ApplyLocationForce(FVector Force, FVector Position)
{
	if (PhysicMesh)
	{
		PhysicMesh->AddForceAtLocation(Force, Position);
	}
}

void AVehicle::UpdateStaticLoads()
{
	VehicleWeight = VehicleSprungMass * Gravity;
	StaticFrontLoad = VehicleWeight * (DistanceOfCentreOfGravityToRearAxis / WheelBaseLength);
	StaticRearLoad = VehicleWeight * (DistanceOfCentreOfGravityToFrontAxis / WheelBaseLength);
}

float AVehicle::GetDriveForce(const float Torque) const
{
	return FrontWheelRadius ? Torque * GearRatio * DrivetrainEfficiency * FinalDriveRatio / FrontWheelRadius : 0;
}

float AVehicle::GetTireTraction(UTire* Tire)
{
	float ForwardVelocity = FMath::Abs(FVector::DotProduct(
		PhysicMesh->GetPhysicsLinearVelocity(),
		PhysicMesh->GetForwardVector()
	)) * 0.036f; // Convert to km/h for comparison
	//Calculate the drive force on the tire;
	float TireRPM = CalculateRPM(Tire);
	//Retrieve the torque from the torque curve
	float TorqueIndex = CurveStep != 0 ? CurveStep *FMath::Floor(TireRPM / CurveStep) : 0;
	float Torque = TorqueCurve.Find(TorqueIndex)?TorqueCurve[TorqueIndex] : 0;
	float TireDriveForce = GetDriveForce(Torque);
	float Traction = Tire->GetTraction(TireDriveForce);
	float ForwardForce = FMath::Abs(ForwardVelocity) > FormulaThreshold ?
		Tire->MagicFormula(Traction, FMath::Abs(Tire->GetSlipRatio()))
		: Traction;
	return ForwardForce * CurrentThrottle;
}

float AVehicle::GetTireBrakingForce(UTire* Tire, float DeltaTime)
{
	float SlipRatio = Tire->GetSlipRatio();
	// Limit Braking force by tire load and friction
	float TireGrip = Tire->TireLoad * Tire->GetFrictionCoefficient();
	// Calculate braking force from slip ratio 
	float MaxBrakingForce = FMath::Abs(Tire->MagicFormula(TireGrip, SlipRatio));

	return FMath::Min(CurrentBrakingForce, GetMinimumWheelForce(Tire, MaxBrakingForce, DeltaTime)) * CurrentBrake;
}

float AVehicle::GetTireRollingResistance(UTire* Tire, float DeltaTime)
{
	return GetMinimumWheelForce(Tire, Tire->GetRollingResistance(), DeltaTime);
}

float AVehicle::GetMinimumWheelForce(UTire* Tire, const float StoppingForce, const float DeltaTime)
{
	FVector WheelForward = Tire->GetForwardVector();
	FVector VelocityAtWheel = PhysicMesh->GetPhysicsLinearVelocityAtPoint(SkeletalMesh->GetSocketLocation(Tire->SocketName));
	//Get the forward force on the wheel
	float SpeedAtWheel = FMath::Abs(FVector::DotProduct(VelocityAtWheel, WheelForward));

	// Calculate the mass on the wheel from its load
	float WheelMass = Gravity != 0 ? Tire->TireLoad / Gravity : 0;

	// Calculate the deceleration on the wheel
	float MaxDeceleration = WheelMass != 0 ? StoppingForce / WheelMass : 0;

	// Velocity change this frame from force
	float MaxVelocityChange = MaxDeceleration * DeltaTime;

	// Use the minimum needed velocity change
	float ActualVelocityChange = FMath::Min(MaxVelocityChange, SpeedAtWheel);

	// Convert back to force: F = ma
	float TotalBrakingForce = DeltaTime != 0 ? ActualVelocityChange * WheelMass / DeltaTime : 0;

	return TotalBrakingForce;
}

float AVehicle::RoundToDecimalPoint(const float Value, const int Points)
{
	int Denominator = FMath::Pow(10, Points > 0 ? Points : 1.0f);
	return FMath::RoundToFloat(Value * Denominator) / Denominator;
}

float AVehicle::CalculateRPM(UTire* Tire) const
{
	return Tire->GetRotationalVelocity() * GearRatio * FinalDriveRatio * 60 / 2 * PI;
}
