// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h" 

// Sets default values
AVehicle::AVehicle()
{
	if (GetWorld())
	{
		Gravity = FMath::Abs(GetWorld()->GetGravityZ());
	}
	//Create the presets
	VehicleSettings = FVehiclePresets();
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
		PhysicMesh->SetMassOverrideInKg(NAME_None, CurrentPresets.TotalVehicleMass, true);

		//For Displaying visual effect
		VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
		VisualMesh->SetupAttachment(PhysicMesh);
		VisualMesh->SetSimulatePhysics(false);

		//Get the original position of the Visual mesh
		DefaultVisualMeshPosition = VisualMesh->GetRelativeLocation();

		//Create the front tires
		FrontRightTire = CreateDefaultSubobject<UTire>(TEXT("Front-Right Tire"));

		FrontLeftTire = CreateDefaultSubobject<UTire>(TEXT("Front-Left Tire"));

		//Create the rears tires
		RearRightTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Right Tire"));

		RearLeftTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Left Tire"));

		CreateTires();
	}
}
// Called when the game starts or when spawned
void AVehicle::BeginPlay()
{
	PhysicMesh->SetMassOverrideInKg(NAME_None, CurrentPresets.TotalVehicleMass, true);
	//Store the spawning information
	SpawnLocation = GetActorLocation();
	SpawnDirection = GetActorQuat();
	//
	MeshDimension = GetMeshDimensions(PhysicMesh);
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
// Called every frame
void AVehicle::Tick(float DeltaTime)
{
	GEngine->AddOnScreenDebugMessage(17, 3.f, FColor::Purple, FString::Printf(TEXT("Debug mode:%f N"), DebugSetting));

	if (DebugSetting != 0)
	{
		FakeAcceleration += DebugSetting > 0 ? 1 : -1;
		DebugSetting = 0;
	}

	float ForwardVelocity = FMath::Abs(FVector::DotProduct(CurrentVelocity, PhysicMesh->GetForwardVector() * 0.036));
	IsUsingMagicFormula = ForwardVelocity > FormulaThreshold;

	Super::Tick(DeltaTime);
	CurrentSteeringAngle = FMath::FInterpTo(CurrentSteeringAngle, CurrentSteering * MaxSteeringAngle, DeltaTime, SteeringInterpSpeed);

	bool IsThrottleActive = FMath::Abs(CurrentThrottle) > 0.08f;
	bool IsSteeringActive = FMath::Abs(CurrentSteering) > 0.08f;
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

	if (GEngine)
	{
		float ForwardAcceleration = FMath::Abs(FVector::DotProduct(Acceleration, PhysicMesh->GetForwardVector() * 0.01));
		GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Green, FString::Printf(TEXT("Speed %f km/h"), ForwardVelocity));
		GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Green, FString::Printf(TEXT("Acceleration %f m/s^2"), ForwardAcceleration));
		GEngine->AddOnScreenDebugMessage(3, 3.f, FColor::Green, FString::Printf(TEXT("Weight %f N"), CurrentPresets.TotalVehicleMass * Gravity));
		GEngine->AddOnScreenDebugMessage(4, 3.f, FColor::Green, FString::Printf(TEXT("Gear: %i: %f"), GearIndex, GearRatio));
	}
	if (PhysicMesh)
	{
		CalculateResistiveForces(CurrentVelocity, DeltaTime);

		//Toggle brakes base on the current braking input 
		IsBraking = CurrentBrake > 0.0f;

		CurrentDrivingForce = 0.0f;
		if (!IsUsingMagicFormula && FMath::Abs(CurrentSteeringAngle) > KINDA_SMALL_NUMBER)
		{
			float LongSpeed = FVector::DotProduct(CurrentVelocity, PhysicMesh->GetForwardVector());
			const float DeltaRad = FMath::DegreesToRadians(CurrentSteeringAngle);
			const float YawRate = FMath::Abs(LongSpeed) * FMath::Tan(DeltaRad) / CurrentPresets.WheelBaseLength;
			PhysicMesh->SetPhysicsAngularVelocityInDegrees(FVector(0, 0, FMath::Sign(LongSpeed) * YawRate));
		}

		for (int i = 0; i < AllTires.Num(); i++)
		{
			UTire* Tire = AllTires[i];
			//If we have a vaild tire
			if (Tire)
			{
				//Update the steering of the wheel
				Tire->UpdateSteering(CurrentSteeringAngle);
				FVector WheelForward = Tire->GetForwardVector();
				FVector VelocityAtWheel = PhysicMesh->GetPhysicsLinearVelocityAtPoint(SkeletalMesh->GetSocketLocation(Tire->SocketName));
				float SpeedAtWheel = FVector::DotProduct(VelocityAtWheel, WheelForward);


				//---------------------------Update wheel parameters------------------------
				float EngineForce = GetDriveForce(GetWheelTorque(Tire)) - (Tire->GetBrakingForce() * CurrentBrake + GetTireRollingResistance(Tire, WheelForward, DeltaTime)) * FMath::Sign(SpeedAtWheel);
				UpdateWheel(Tire, EngineForce, DeltaTime);
				//--------------------------- end ------------------------------------------

				//--------------------------- Handle forces --------------------

				float LongitudinalForce = GetTireDriveForce(Tire) - (GetUnSignedTireBrakingForce(Tire, DeltaTime) + GetTireRollingResistance(Tire, WheelForward, DeltaTime)) * FMath::Sign(SpeedAtWheel);
				FVector LateralFriction = Tire->GetLateralForceVector(DeltaTime);

				//Apply friction circle for
				float GripLimit = Tire->GetMaxGrip();
				float CombinedGrip = FMath::Sqrt(FMath::Pow(LateralFriction.Size(), 2) + FMath::Pow(LongitudinalForce, 2));
				//Scale down the values if there surpass the grip limit
				if (CombinedGrip > GripLimit)
				{
					const float Scale = GripLimit / CombinedGrip;
					LongitudinalForce *= Scale;
					LateralFriction *= Scale;
				}


				ApplyWheelForce(Tire, LongitudinalForce, WheelForward, false, false, true);

				//Apply the lateral friction force
				ApplyLocationForce(LateralFriction, Tire->GetContactPoint(), true, false, false);


				//--------------------------- end ------------------------------------------

				float TireLoad = Tire->TireLoad;
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(4 + i, 3.f, FColor::Green,
						FString::Printf(TEXT("%s: slip %.3f rad, force %.0f, load %.0f, grounded %i, X Difference %f , Y Difference %f, Z Difference %f"),
							*Tire->GetName(), Tire->GetPeakSlips(WHEELMODE::BRAKING), LateralFriction.Size(),
							Tire->TireLoad, Tire->IsGrounded, PhysicMesh->GetCenterOfMass().X - Tire->GetContactPoint().X,
							PhysicMesh->GetCenterOfMass().Y - Tire->GetContactPoint().Y, PhysicMesh->GetCenterOfMass().Z - Tire->GetContactPoint().Z));
			}
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
		FVector StartLocation = Tire->GetComponentLocation();
		FVector EndLocation = StartLocation + (RayDirection * (Wheel.SuspensionLength + Wheel.WheelRadius));

		FHitResult Hit;
		FCollisionQueryParams QueryParameters;
		QueryParameters.bReturnPhysicalMaterial = true;
		QueryParameters.AddIgnoredActor(this);

		//Perform the Ray cast
		if (GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, TraceChannel, QueryParameters))
		{
			//Calculate compression using distance from mount point to the ground hit point
			float CurrentDistance = RoundToDecimalPoint(FVector::Dist(StartLocation, Hit.Location));
			CurrentDistance = FMath::Min(CurrentDistance, Wheel.SuspensionLength);
			float Error = FMath::FGenericPlatformMath::Max(Wheel.WheelRadius - CurrentDistance, 0);

			//Get the compression of the wheel
			float Compression = Tire->GetCompression(Error - GroundOffset);

			Tire->IsGrounded = true;

			// We need the velocity of the vehicle at the specific point where the suspension is attached
			FVector VelocityAtPoint = PhysicMesh->GetPhysicsLinearVelocityAtPoint(StartLocation) +
				FVector::CrossProduct(PhysicMesh->GetPhysicsAngularVelocityInRadians(), PhysicMesh->GetComponentLocation() - StartLocation);

			// The damping force is based on the velocity along the suspension axis (Up Vector)
			float SuspensionVelocity = FVector::DotProduct(VelocityAtPoint, Hit.Normal);
			//Update the suspension force of the tire
			Tire->CalculateSuspensionForce(SuspensionVelocity);
			//Update the suspension for the model
			float SpringForce = Error * SpringStiffness;
			float DampingForce = FVector::DotProduct(VelocityAtPoint, FVector::UpVector) * LongitudinalDamping;
			FVector ForceZ = Hit.Normal * FMath::Max(SpringForce - DampingForce, 0);



			//Store the hit location and normal
			Tire->StoreTireContactInformation(Hit);
			Tire->UpdateRollingRadius(StartLocation);

			// Update the tire friction
			UPhysicalMaterial* PhysMat = Hit.PhysMaterial.Get();

			if (PhysMat)
			{
				float Friction = PhysMat->Friction;
				Tire->UpdateFrictionCoefficient(Friction);
			}
			//Apply the suspension to the model
			ApplyLocationForce(ForceZ, Tire->GetContactPoint(), false, false);
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
		if (PauseAction)
		{
			// Bind Pausing
			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AVehicle::Input_Pause);
		}
	}

}

void AVehicle::ResetVehiclePosition()
{
	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnDirection);
	PhysicMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PhysicMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	for (UTire* Tire : AllTires)
	{
		Tire->ResetRotationalVelocity();
	}
}

void AVehicle::BlueprintTogglePauseMenu()
{
	TogglePauseMenu();
}

void AVehicle::SetFromPreset(const int Index)
{
	//Get the preset
	CurrentPresets = VehicleSettings.GetPreset(Index);
	GearIndex = 0;
	//Apply the settings
	GearIndex = 0;
	GearRatio = CurrentPresets.GearRatios[0];
	LongitudinalDamping = CurrentPresets.TotalVehicleMass * 5;
	PhysicMesh->SetMassOverrideInKg(NAME_None, CurrentPresets.TotalVehicleMass, true);
	TargetHeight = CurrentPresets.RearWheelRadius;
	SpringStiffness = CurrentPresets.TotalVehicleMass * 20;
	GroundOffset = CurrentPresets.TotalVehicleMass * Gravity / (4 * SpringStiffness);

	if (MeshDimension.X != 0 && MeshDimension.Y != 0 && MeshDimension.Z != 0)
	{
		MeshScale = FVector(
			CurrentPresets.WheelBaseLength / MeshDimension.X,
			CurrentPresets.Width / MeshDimension.Y,
			CurrentPresets.Height / MeshDimension.Z);
		PhysicMesh->SetWorldScale3D(MeshScale);
		VisualMesh->SetWorldScale3D(MeshScale);
		SkeletalMesh->SetWorldScale3D(MeshScale);
		MaxSuspensionLength = CurrentPresets.Height * 0.625;
	}

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
	if (!IsGearChanging)
	{
		if (Switch > 0 && GearIndex + 1 < CurrentPresets.GearRatios.Num())
		{
			GearIndex++;
		}
		else if (Switch < 0 && GearIndex - 1 >= 0)
		{
			GearIndex--;
		}
	}
	IsGearChanging = Value.Get<float>() != 0;
	GearRatio = CurrentPresets.GearRatios[GearIndex];
}

void AVehicle::Input_Pause(const FInputActionValue& Value)
{

	//Pause the program the first frame the pause button is down

	TogglePauseMenu();


}

FVector AVehicle::CreateVehicleTorque(FVector Force, FVector Position)
{
	// Calculate moment arm to the position
	FVector FrontMomentArm = Position - PhysicMesh->GetCenterOfMass();
	// Calculate the counter torque created
	FVector  ArmTorque = FVector::CrossProduct(Force, FrontMomentArm);
	return ArmTorque;
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
				FName(SocketNames[i])
			);

			Tire->WheelMesh->SetupAttachment(
				SkeletalMesh,
				FName(SocketNames[i])
			);

			//Update the configuration of the wheel
			if (CurrentPresets.SocketToWheelConfigurations.Find(SocketNames[i]))
			{
				Tire->UpdateWheelConfiguration(CurrentPresets.SocketToWheelConfigurations[SocketNames[i]]);

			}
			Tire->StoreInitialTireMeshDimensions();
			//Setup the suspension
			const bool IsFrontWheel = !Tire->WheelConfig.IsRearWheel;
			//Update the suspension
			Tire->UpdateSuspension(IsFrontWheel ? CurrentPresets.FrontSuspensionStiffness : CurrentPresets.RearSuspensionStiffness,
				IsFrontWheel ? CurrentPresets.FrontSuspensionDamping : CurrentPresets.RearSuspensionDamping,
				MaxSuspensionLength,
				IsFrontWheel ? CurrentPresets.FrontTireVerticalStiffness : CurrentPresets.RearTireVerticalStiffness,
				IsFrontWheel ? CurrentPresets.FrontUnsprungMass : CurrentPresets.RearUnsprungMass, (IsFrontWheel ? StaticFrontLoad : StaticRearLoad) / 2);
			//Update the radius and friction of the wheel based on position
			Tire->UpdateTireRadius(IsFrontWheel ? CurrentPresets.FrontWheelRadius : CurrentPresets.RearWheelRadius);
			Tire->UpdateTireFrictionCoefficient(IsFrontWheel ? CurrentPresets.FrontTireFriction : CurrentPresets.RearTireFriction);
			Tire->UpdateRollingResistanceCoefficient(IsFrontWheel ? CurrentPresets.FrontRollingResistanceCoefficient : CurrentPresets.RearRollingResistanceCoefficient);
			Tire->UpdateWheelInertia(IsFrontWheel ? CurrentPresets.FrontWheelInertia : CurrentPresets.RearWheelInertia);
			Tire->UpdateBrakingTorque(IsFrontWheel ? CurrentPresets.FrontBrakeTorque : CurrentPresets.RearBrakeTorque);
			Tire->UpdateWheelFeatures(CurrentPresets.StiffnessFactors, CurrentPresets.ShapeFactors, CurrentPresets.CurvatureFactors);
			//Store the socket name with wheel
			Tire->SocketName = SocketNames[i];
			Tire->UpdateWheelWorldPosition(MeshScale);

		}
	}
}

void AVehicle::CalculateSuspensionDynamics(float DeltaTime,
	float LongitudinalAcceleration, const float LateralAcceleration)
{
	if (DeltaTime != 0)
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
}

void AVehicle::CalculateResistiveForces(FVector Velocity, float DeltaTime)
{
	FVector MovementDirection = -Velocity.GetSafeNormal();
	//Calculate drag using: Drag force = 0.5*drag Coefficient*Area*air density* speed^2
	float CurrentSpeed = Velocity.Size() / 100.0f;
	float Area = CurrentPresets.Height * CurrentPresets.Width / 10000;
	//Calculate drag in Newtons
	FVector DragForce = MovementDirection * 0.5 * CurrentPresets.DragCoefficient * Area * AirDensity * CurrentSpeed * CurrentSpeed;
	// Convert Newtons to Unreal force units (kg·cm/s²)
	DragForce *= 100;
	//Apply drag
	PhysicMesh->AddForce(DragForce, NAME_None, false);
	SumOfResistiveForces = 0.0f;
	SumOfResistiveForces = DragForce.Size();
}

void AVehicle::CalculatePitchWeightTransfer(const float LongitudinalAcceleration, const float LateralAcceleration)
{
	// Dynamic weight transfer due to longitudinal acceleration
	// Weight transfer = (Mass × Acceleration × CG Height) / Wheelbase
	float LongitudinalWeightTransfer = (CurrentPresets.VehicleSprungMass * LongitudinalAcceleration * CurrentPresets.CentreOfGravityHeight) / CurrentPresets.WheelBaseLength;
	float LateralForceOnVehicle = LateralAcceleration * CurrentPresets.VehicleSprungMass;
	float LateralChangeInTireLoad = CurrentPresets.TrackWidth > 0 ? LateralForceOnVehicle * (CurrentPresets.CentreOfGravityHeight / CurrentPresets.TrackWidth) : 0;
	// During acceleration (positive), weight transfers to rear
	// During braking (negative), weight transfers to front
	float DynamicFrontLoad = StaticFrontLoad - LongitudinalWeightTransfer;
	float DynamicRearLoad = StaticRearLoad + LongitudinalWeightTransfer;

	// Add pitch-induced weight redistribution
	// Pitch affects load distribution: nose up = more rear load
	float PitchWeightTransfer = (CurrentPresets.VehicleSprungMass * Gravity * sin(PitchAngle) * CurrentPresets.CentreOfGravityHeight) / CurrentPresets.WheelBaseLength;
	DynamicFrontLoad -= PitchWeightTransfer;
	DynamicRearLoad += PitchWeightTransfer;

	// Add suspension force contribution and store final dynamic loads
	FrontDynamicLoad = FMath::Max(0.0f, DynamicFrontLoad);
	RearDynamicLoad = FMath::Max(0.0f, DynamicRearLoad);
	//Update the normal force on the tires
	for (UTire*& Tire : AllTires)
	{
		//Get the combination of the static and longitudinal weight transfer for the tire.
		float DynamicTireLoad = (!Tire->WheelConfig.IsRearWheel ? FrontDynamicLoad : RearDynamicLoad) / 2;
		//Add the lateral load
		DynamicTireLoad += LateralChangeInTireLoad * (Tire->WheelConfig.IsRightWheel ? -1 : 1) / 2;
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
			float UnsprungMass = !Tire->WheelConfig.IsRearWheel ? CurrentPresets.FrontUnsprungMass : CurrentPresets.RearUnsprungMass;

			// Get suspension force
			float SuspensionForce = Tire->GetSuspensionForce();

			// Net force on unsprung mass = Tire force (tire load) - Suspension force
			float NetUnsprungForce = Tire->TireLoad - SuspensionForce;

			// Update unsprung mass dynamics
			float UnsprungAcceleration = UnsprungMass != 0 ? NetUnsprungForce / UnsprungMass : 0;
			//Obtain the suspension forces from the wheels and update the unsprung mass positions
			if (!Tire->WheelConfig.IsRearWheel)
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
			if (!Tire->WheelConfig.IsRearWheel)
				FrontUnsprungVelocity *= FMath::Exp(-CurrentPresets.FrontUnSprungDamping * DeltaTime);
			else
				RearUnsprungVelocity *= FMath::Exp(-CurrentPresets.RearUnSprungDamping * DeltaTime);
		}
	}
}

void AVehicle::CalculatePitchAndHeaveDynamics(float DeltaTime, float  LongitudinalAcceleration)
{
	//Use the suspension forces to obtain the pitch moment
	float PitchMoment = (FrontUnsprungForce * CurrentPresets.DistanceOfCentreOfGravityToFrontAxis) - (RearUnsprungForce * CurrentPresets.DistanceOfCentreOfGravityToRearAxis);

	GEngine->AddOnScreenDebugMessage(11, 3.f, FColor::Green, FString::Printf(TEXT("Default pitch: %f N"), PitchMoment));
	float WeightTransferMoment = CurrentPresets.VehicleSprungMass * LongitudinalAcceleration * CurrentPresets.CentreOfGravityHeight;
	PitchMoment += WeightTransferMoment;

	float PitchRestoringMoment = -(CurrentPresets.PitchStiffness + CurrentPresets.VehicleSprungMass * Gravity * CurrentPresets.CentreOfGravityHeight) * FMath::Sin(PitchAngle);
	GEngine->AddOnScreenDebugMessage(12, 3.f, FColor::Green, FString::Printf(TEXT("Restoring: %f"), PitchMoment));
	PitchMoment += PitchRestoringMoment;
	// Add pitch damping to prevent oscillation
	float PitchDampingTorque = -CurrentPresets.PitchDamping * PitchVelocity;

	PitchMoment += PitchDampingTorque;

	//Get the pitch acceleration from the moment
	float PitchAcceleration = CurrentPresets.PitchInertia != 0.0f ? PitchMoment / CurrentPresets.PitchInertia : 0;
	//Update the pitch velocity and angle
	PitchVelocity += PitchAcceleration * DeltaTime;

	//PitchVelocity *= FMath::Exp(PitchAcceleration * PitchDamping * DeltaTime);
	PitchAngle += PitchVelocity * DeltaTime;

	GEngine->AddOnScreenDebugMessage(10, 3.f, FColor::Green, FString::Printf(TEXT("Pitch: %f N"), PitchAngle));

	// Clamp pitch angle to realistic limits
	PitchAngle = FMath::Clamp(PitchAngle, FMath::DegreesToRadians(-15.0f), FMath::DegreesToRadians(15.0f));
	//Get the overall vertical force on the suspension
	float TotalSuspensionForce = FrontUnsprungForce + RearUnsprungForce;

	float NetVerticalForce = TotalSuspensionForce - VehicleWeight;
	//Apply the heave damping effect
	float HeaveDampingForce = -CurrentPresets.HeaveDamping * (HeaveVelocity / DeltaTime);
	NetVerticalForce += HeaveDampingForce;
	// Add restoring force to bring heave back to equilibrium
	float HeaveRestoringForce = (CurrentPresets.FrontSuspensionStiffness + CurrentPresets.RearSuspensionStiffness) * HeavePosition;
	NetVerticalForce -= HeaveRestoringForce;

	//Calculate the heave acceleration with acceleration = force/mass
	float HeaveAcceleration = CurrentPresets.VehicleSprungMass != 0 ? NetVerticalForce / CurrentPresets.VehicleSprungMass : 0;
	// Update heave velocity and position
	HeaveVelocity += HeaveAcceleration * DeltaTime;
	HeavePosition += HeaveVelocity * DeltaTime;

	// Clamp to realistic limits
	HeavePosition = FMath::Clamp(HeavePosition, -10.0f, 10.0f);
}

void AVehicle::UpdateWheel(UTire* Tire, const float LongitudinalForceMagnitude, float DeltaTime)
{
	FVector SocketLocation = SkeletalMesh->GetSocketLocation(Tire->SocketName);
	//Get the lateral and longitudinal directions of the wheel
	FVector WheelForward = Tire->GetForwardVector();
	FVector WheelUp = Tire->GetUpVector();
	FVector WheelRight = FVector::CrossProduct(WheelUp, WheelForward);
	//Get the wheel's velocity
	FVector VelocityAtWheel = PhysicMesh->GetPhysicsLinearVelocityAtPoint(Tire->GetContactPoint());
	//Calculate the lateral and longitudinal speed of the wheel
	float  LongitudinalVelocity = FVector::DotProduct(VelocityAtWheel, WheelForward);
	float LateralVelocity = FVector::DotProduct(VelocityAtWheel, WheelRight);

	//Update the rotational velocity

	Tire->UpdateWheelRotationalVelocity(LongitudinalForceMagnitude, Tire->GetBrakingForce(), CurrentBrake, LongitudinalVelocity, DeltaTime);


	//Update the slip angle and ratio 
	Tire->UpdateSlipAngle(LongitudinalVelocity, LateralVelocity);

	if (Tire->WheelConfig.IsDriveWheel)
	{
		float EffectiveWheelRadius = Tire->GetRollingRadius();
		GEngine->AddOnScreenDebugMessage(19, 3.f, FColor::Green, FString::Printf(TEXT("Speed: %f"), LongitudinalVelocity));
		GEngine->AddOnScreenDebugMessage(20, 3.f, FColor::Green, FString::Printf(TEXT("Anglar: %f"), Tire->GetRotationalVelocity() * EffectiveWheelRadius));
	}
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

void AVehicle::ApplyWheelForce(UTire* Tire, float ForceMagnitude, FVector Direction, const bool HasXTorque, const bool HasYTorque, const bool HasZTorque)
{
	if (abs(ForceMagnitude) > 0)
	{
		ApplyLocationForce(ForceMagnitude * Direction, Tire->GetContactPoint(), HasZTorque, HasXTorque, HasYTorque);
	}
}

void AVehicle::ApplyLocationForce(FVector Force, FVector Position, const bool HasZTorque, const bool HasXTorque, const bool HasYTorque)
{
	if (PhysicMesh)
	{
		FVector CounterTorque = CreateVehicleTorque(Force, Position);
		//Allow X-axis torque
		if (HasXTorque)
			CounterTorque.X = 0;
		//Allow Z-axis torque
		if (HasZTorque)
			CounterTorque.Z = 0;
		//Allow Y-axis torque
		if (HasYTorque)
			CounterTorque.Y = 0;

		PhysicMesh->AddTorqueInRadians(CounterTorque);

		PhysicMesh->AddForceAtLocation(Force, Position);
	}
}

void AVehicle::UpdateStaticLoads()
{
	VehicleWeight = CurrentPresets.VehicleSprungMass * Gravity;
	StaticFrontLoad = VehicleWeight * (CurrentPresets.DistanceOfCentreOfGravityToRearAxis / CurrentPresets.WheelBaseLength);
	StaticRearLoad = VehicleWeight * (CurrentPresets.DistanceOfCentreOfGravityToFrontAxis / CurrentPresets.WheelBaseLength);
}

float AVehicle::GetDriveForce(const float Torque) const
{
	return CurrentPresets.FrontWheelRadius > 0 ? Torque * CurrentThrottle * GearRatio * CurrentPresets.DrivetrainEfficiency * CurrentPresets.FinalDriveRatio / CurrentPresets.FrontWheelRadius : 0;
}

float AVehicle::GetTireDriveForce(UTire* Tire)
{
	if (Tire->WheelConfig.IsDriveWheel)
	{
		float DriveForce = GetDriveForce(GetWheelTorque(Tire));
		if (IsUsingMagicFormula)
		{
			DriveForce = Tire->MagicFormula(FMath::Abs(DriveForce), Tire->GetSlipRatio(), WHEELMODE::ACCELERATION);
			//Prevent reversing of drive force  from throttle direction
			if (FMath::Sign(DriveForce) != FMath::Sign(CurrentThrottle))
				DriveForce = 0;
		}
		return DriveForce;
	}
	return 0;
}

float AVehicle::GetMaximumSlipBasedBreakingForce(UTire* Tire, float DeltaTime)
{
	float SlipRatio = Tire->GetSlipRatio();
	// Limit Braking force by tire load and friction
	float TireGrip = Tire->GetBrakingForce();
	// Calculate braking force from slip ratio 
	float MaxBrakingForce = FMath::Abs(Tire->MagicFormula(TireGrip, SlipRatio, WHEELMODE::BRAKING));
	return MaxBrakingForce;
}

float AVehicle::GetUnSignedTireBrakingForce(UTire* Tire, float DeltaTime)
{
	return GetMaximumSlipBasedBreakingForce(Tire, DeltaTime) * CurrentBrake;
}

float AVehicle::GetTireRollingResistance(UTire* Tire, FVector WheelDirection, float DeltaTime)
{
	return GetMinimumWheelForce(Tire, Tire->GetRollingResistance(), WheelDirection, DeltaTime);
}

float AVehicle::GetMinimumWheelForce(UTire* Tire, const float StoppingForce, FVector WheelDirection, const float DeltaTime)
{
	FVector VelocityAtWheel = PhysicMesh->GetPhysicsLinearVelocityAtPoint(SkeletalMesh->GetSocketLocation(Tire->SocketName));
	//Get the forward force on the wheel
	float SpeedAtWheel = FMath::Abs(FVector::DotProduct(VelocityAtWheel, WheelDirection));

	// Calculate the mass on the wheel from its load
	float WheelMass = Gravity != 0 ? Tire->TireLoad / Gravity : 0;

	// Calculate the deceleration on the wheel
	float MaxDeceleration = WheelMass != 0 ? FMath::Abs(StoppingForce) / WheelMass : 0;

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
	//Calculate the RPM
	float RPM = (FMath::Abs(Tire->GetRotationalVelocity()) * GearRatio * CurrentPresets.FinalDriveRatio * 60 / (2 * PI));
	//Combine with the minimum torque need to move the vehicle
	return CurrentPresets.MinimumStartingRPM + RPM;
}

float  AVehicle::GetWheelTorque(UTire* Tire) const
{
	float TireRPM = CalculateRPM(Tire);
	//Retrieve the torque from the torque curve
	float TorqueIndex = CurrentPresets.CurveStep != 0 ? CurrentPresets.CurveStep * FMath::Floor(TireRPM / CurrentPresets.CurveStep) : 0;
	float Torque = 0;
	if (CurrentPresets.TorqueCurve.Find(TorqueIndex))
	{
		Torque = CurrentPresets.TorqueCurve[TorqueIndex];
	}
	GEngine->AddOnScreenDebugMessage(18, 3.f, FColor::Green, FString::Printf(TEXT("RPM :%f N"), CalculateRPM(Tire)));
	return Torque;
}