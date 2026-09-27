// Fill out your copyright notice in the Description page of Project Settings.


#include "Tire.h"



UTire::UTire()
{
	WheelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMesh"));
	WheelMesh->SetupAttachment(this);
	WheelMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void UTire::UpdateTireRadius(const float Value)
{
	SuspensionSettings.WheelRadius = Value;
	if (WheelMeshDimension.X != 0 && WheelMeshDimension.Y != 0 && WheelMeshDimension.Z != 0 && WheelMesh)
	{
		FVector NewScale = FVector(
			Value / WheelMeshDimension.X,
			0.5,
			Value / WheelMeshDimension.Z);

		WheelMesh->SetRelativeScale3D(NewScale);
	}
}

void UTire::UpdateMaxGrip()
{
	MaxGrip = TireGrip * TireLoad;
}

float UTire::GetBrakingForce() const
{
	//Calculate the braking force from the torque based
	float BrakingForce = SuspensionSettings.WheelRadius != 0 ? BrakingTorque / SuspensionSettings.WheelRadius : 0;
	return BrakingForce;
}

float UTire::CalculatePeakSlip(const int Index) const
{
	//Clamp the index to a useable range
	const int FormulaIndex = FMath::Clamp(Index, 0, TireFormulas.Num() - 1);
	//Require a existing a Tire formula to estimate its peak
	if (TireFormulas.Num() > 0)
	{
		MagicFormulaModel SelectedTireFormula = TireFormulas[FormulaIndex];
		const float B = SelectedTireFormula.StiffnessFactor;
		const float C = SelectedTireFormula.ShapeFactor;
		const float E = SelectedTireFormula.CurvatureFactor;
		const float Sh = SelectedTireFormula.HorizontalShift;

		// For a shape factor of  less than one or a negative stiffness factor the tire 
		// curve never reaches the sine's peak at finite slip
		if (B <= 0.0f || C <= 1.0f) return 0.0f;

		const float Target = FMath::Tan(0.5f * PI / C);

		//Constrain the curvature factor to 1 or less
		if (E >= 1.0f)
		{
			const float U = 1.0f / FMath::Sqrt(E - 1.0f);
			const float GMax = U * (1.0f - E) + E * FMath::Atan(U);
			if (GMax < Target)
			{
				return FMath::Max(U / B - Sh, 0.0f);
			}
		}

		// Seed with the E = 0 solution, estimate the value using Newton–Raphson
		// over six iterations
		float X = Target / B;
		for (int i = 0; i < 6; ++i)
		{
			const float F = B * X * (1.0f - E) + E * FMath::Atan(B * X) - Target;
			const float dF = B * (1.0f - E) + E * B / (1.0f + FMath::Square(B * X));
			if (FMath::Abs(dF) < SMALL_NUMBER) break;
			X -= F / dF;
		}
		return FMath::Max(X - Sh, 0.0f);
	}
	return 0;
}

float UTire::GetPeakSlips(const int Index)
{
	if (Index >= 0 && Index < PeakSlips.Num())
	{
		return PeakSlips[Index] + TireFormulas[Index].HorizontalShift;
	}
	return 0.0f;
}
FVector UTire::GetLateralForceVector(const float DeltaTime) const
{
	if (!IsGrounded || FMath::Abs(SlipAngle) < KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	// Get the tire's local coordinate system
	FVector TireForward = GetForwardVector();
	FVector TireUp = GetUpVector();
	FVector TireRight = FVector::CrossProduct(TireUp, TireForward);

	//Get the mass on the wheel
	float WheelMass = Gravity != 0 ? TireLoad / Gravity : 0;
	//Get the maximum lateral force currently acting on the wheel
	float MaxLateralForce = DeltaTime > 0 ? FMath::Abs(LastLateralVelocity) * WheelMass / DeltaTime : 0;
	//Get the cornering force the wheel can currently produce
	float CorneringForce = FMath::Abs(MagicFormula(MaxGrip, SlipAngle, WHEELMODE::CORNERING));
	//Limit the lateral force the wheel provides 
	float LateralMagnitude = FMath::Min(CorneringForce, MaxLateralForce);

	// Apply force opposite to slip direction
	float ForceDirection = -FMath::Sign(SlipAngle);

	return TireRight * (ForceDirection * LateralMagnitude);
}

void UTire::UpdateTireFrictionCoefficient(const float NewValue)
{
	TireFrictionCoefficient = NewValue;
	UpdateTotalGrip();
}

void UTire::UpdateFrictionCoefficient(const float NewValue)
{
	FrictionCoefficient = NewValue;
	UpdateTotalGrip();
}

void UTire::UpdateSteering(const float NewAngle)
{

	//Only allow steering from the front tires
	SteerAngle = WheelConfig.IsSteerWheel ? NewAngle : 0;
	//Rotate the tire to the new steer angle

	if (WheelMesh)
		WheelMesh->SetRelativeRotation(FRotator(0.0f, SteerAngle, 0.0f));
	SetRelativeRotation(FRotator(0.0f, SteerAngle, 0.0f));
}


void UTire::UpdateTireLoad(float NewWeight)
{
	TireLoad = FMath::Max(0.0f, NewWeight) + SuspensionSettings.UnSpringMass * Gravity;
	//Prevent negative tire load and tire load for aerial tires
	if (!IsGrounded)
	{
		TireLoad = 0;
	}
	//Update the maximum traction force on the tire with the new tire load
	UpdateMaxGrip();
}

void UTire::UpdateSlipRatio(const float CurrentRotationalVelocity, const float VehicleSpeedAtWheel, const bool IsBraking)
{
	float WheelSurfaceSpeed = CurrentRotationalVelocity * SuspensionSettings.WheelRadius;
	float Denominator = FMath::Abs(IsBraking ? VehicleSpeedAtWheel : VehicleSpeedAtWheel);
	SlipRatio = FMath::Abs(Denominator) > 0.1f ? (WheelSurfaceSpeed - VehicleSpeedAtWheel) / Denominator : 0;
}

void UTire::UpdateSlipAngle(const float LongitudinalVelocity, const float LateralVelocity)
{
	const float Denominator = FMath::Max(FMath::Abs(LongitudinalVelocity), MinSlipSpeed);
	SlipAngle = FMath::Atan2(LateralVelocity, Denominator);
	LastLateralVelocity = LateralVelocity;
}

void UTire::UpdateWheelFeatures(const TArray<MagicFormulaModel> NewTireFormulas)
{
	TireFormulas.Empty();
	PeakSlips.Empty();
	TireFormulas = NewTireFormulas;
	for (int i = WHEELMODE::ACCELERATION; i <= WHEELMODE::CORNERING; i++)
	{
		PeakSlips.Add(CalculatePeakSlip(i));
	}
}

void UTire::UpdateWheelWorldPosition(FVector const MeshScale)
{
	if (MeshScale.X != 0 && MeshScale.Y != 0 && MeshScale.Z != 0)
	{
		SetRelativeLocation(WheelConfig.Position / MeshScale);
		WheelMesh->SetRelativeLocation(WheelConfig.Position / MeshScale);
	}
}

void UTire::ClampToVehicleWheelSpeed(const float WheelSpeed, const float DeltaTime, const bool IsBraking)
{
	float Radius = SuspensionSettings.WheelRadius;
	//If the tire is in the air skip
	if (!IsGrounded)
	{
		WheelRotationalVelocity *= FMath::Pow(WheelDamper, DeltaTime);
		return;
	}

	WheelRotationalVelocity = Radius != 0 ? WheelSpeed * (GetPeakSlips(WHEELMODE::ACCELERATION) + 1) / Radius : 0;
	UpdateSlipRatio(WheelRotationalVelocity, WheelSpeed, IsBraking);
}

void UTire::StoreInitialTireMeshDimensions()
{
	WheelMeshDimension = GetMeshDimensions(WheelMesh);
}

void UTire::UpdateLastLongitudinalAndLateralForce(const float XForce, const float YForce)
{
	LastLongitudinalForce = XForce; LastLateralForce = YForce;
}

float UTire::GetRollingResistance() const
{
	return IsGrounded ? TireLoad * RollingResistanceCoefficient : 0;
}

float UTire::CalculateCompression(const float CurrentDistance)
{
	//Calculate the spring compression using the difference between the suspension 
	//length and the bottom of the wheel
	SuspensionCompression = FMath::Max(0.0f, SuspensionSettings.RestPosition + CurrentDistance);
	return SuspensionCompression;
}


float UTire::CalculateSuspensionForce(const float SuspensionVelocity)
{
	float SpringForce = SuspensionCompression * SuspensionSettings.SpringStiffness;
	//Damping = suspension velocity* Damping coefficient
	float DampingForce = SuspensionVelocity * SuspensionSettings.DampingCoefficient;
	// Total Force = Spring force - Damping (Damping opposes the velocity)
	SuspensionForce = SpringForce - DampingForce;
	//Clamp the total force to prevent negative values 
	SuspensionForce = FMath::Max(0.0f, SuspensionForce);
	return IsGrounded ? SuspensionForce : 0;
}

float UTire::GetNormalForce() const
{
	return IsGrounded ? TireLoad : 0;
}
float UTire::CalculateRotationalAcceleration(const float CurrentWheelRotationalVelocity, const float LongitudinalForceMagnitude, const float ResistiveForce, const float Multiplier, const float GroundSpeed, const float DeltaTime) const
{
	float RotationSign = CurrentWheelRotationalVelocity >= 0 ? 1 : -1;
	float Radius = SuspensionSettings.WheelRadius;
	if (Inertia != 0 && Radius != 0)
	{
		//Calculate the net force on the wheel
		const float NetForce = LongitudinalForceMagnitude * Multiplier - RotationSign * ResistiveForce;
		//Convert the force in to a torque
		float NetTorque = NetForce * Radius;
		//Calculate the acceleration
		float RotationalVelocityPerFrame = (NetTorque / Inertia) * DeltaTime;
		//Calculate the maximum speed change the wheel can undergo per time period
		//based on the wheel's ground speed and surface speed
		const float SurfaceSpeed = CurrentWheelRotationalVelocity * Radius;
		const float SpeedScale = FMath::Max3(FMath::Abs(SurfaceSpeed), FMath::Abs(GroundSpeed), 10.0f);
		const float MaxSurfaceSpeedChange = DeltaTime * SpeedScale;
		//Clamp the acceleration based on the maximum velocity change
		RotationalVelocityPerFrame = FMath::Clamp(RotationalVelocityPerFrame,
			-MaxSurfaceSpeedChange / Radius, MaxSurfaceSpeedChange / Radius);
		//Prevent the wheel from swapping directions in a single frame
		float NextSign = CurrentWheelRotationalVelocity + RotationalVelocityPerFrame >= 0 ? 1 : -1;
		return  RotationalVelocityPerFrame;
	}
	return 0;
}


void UTire::UpdateWheelRotationalVelocity(const float LongitudinalForceMagnitude, const float MaxBrakingForce, const float BrakeStrength, const float GroundSpeed, const float DeltaTime)
{
	float Radius = SuspensionSettings.WheelRadius;
	if (DeltaTime != 0)
	{
		//If the tire is in the air enter free rotation
		if (!IsGrounded)
		{
			WheelRotationalVelocity *= FMath::Pow(WheelDamper, DeltaTime);
			return;
		}
		float RotationSign = WheelRotationalVelocity >= 0 ? 1 : -1;
		if (Inertia != 0 && Radius != 0)
		{
			float ResistiveForce = 0;
			//The throttle multiplier
			float Multiplier = 1.0;
			//float Error = (SlipRatio - PeakSlips[WHEELMODE::ACCELERATION]);
			//The change in speed to return to the slip ratio peak
			float SpeedError = (GroundSpeed * (PeakSlips[WHEELMODE::ACCELERATION] + 1) - WheelRotationalVelocity * Radius) / Radius;
			//The per frame acceleration need to return to the peak
			float CorrectionAcceleration = SpeedError / DeltaTime;
			//The force need to provide corrective acceleration
			float CorrectiveForce = (CorrectionAcceleration * Inertia / Radius);
			//The direction the corrective force needs to act
			float CorrectiveSign = FMath::Sign(SpeedError);
			//The amount of force available for pulsing the brakes
			float AvailableBrakingForce = MaxBrakingForce * (1 - BrakeStrength);
			//The direction of the throttle
			float ThrottleSign = FMath::Sign(LongitudinalForceMagnitude);

			//If the wheel's rotational velocity needs to be increased and there is a
			//force for doing so accelerate the wheel
			if (ThrottleSign == RotationSign && CorrectiveSign == RotationSign
				&& LongitudinalForceMagnitude != 0)
			{
				Multiplier = FMath::Min(FMath::Abs(CorrectiveForce / (LongitudinalForceMagnitude)), 1);
			}
			//If the wheel's rotational velocity needs to be reduced pulse the brakes
			// and reduce the throttle if needed
			else if (CorrectiveSign * -1 == RotationSign && AvailableBrakingForce != 0)
			{
				//Calculate the maximum force we need or can give for decelerating the wheel
				ResistiveForce = FMath::Min(FMath::Abs(CorrectiveForce), AvailableBrakingForce);
				//If the total corrective force needed force exceeds the pulsing force reduce the throttle
				if (AvailableBrakingForce < LongitudinalForceMagnitude + FMath::Abs(CorrectiveForce) 
					&& CorrectiveSign == ThrottleSign * -1 && LongitudinalForceMagnitude != 0)
				{
					Multiplier = FMath::Max(0, (AvailableBrakingForce - FMath::Abs(CorrectiveForce)) 
						/ FMath::Abs(LongitudinalForceMagnitude));
				}
			}
			//Calculate the acceleration
			float RotationalVelocityPerFrame = CalculateRotationalAcceleration(WheelRotationalVelocity, LongitudinalForceMagnitude, ResistiveForce, Multiplier, GroundSpeed, DeltaTime);


			//Update the rotation velocity with the acceleration
			if (RotationalVelocityPerFrame != 0)
			{
				WheelRotationalVelocity += RotationalVelocityPerFrame;
			}
			//	WheelRotationalVelocity = GroundSpeed * (PeakSlips[WHEELMODE::ACCELERATION] + 1) / Radius;
			UpdateSlipRatio(WheelRotationalVelocity, GroundSpeed, false);
		}
	}
}

float UTire::MagicFormula(const float peakValue, const float x, const int Index) const
{

	int FormulaIndex = FMath::Clamp(Index, 0, TireFormulas.Num() - 1);
	if (TireFormulas.Num() > 0)
	{
		return TireFormulas[FormulaIndex].MagicFormula(peakValue, x);
	}
	return 0;
}

void UTire::StoreTireContactInformation(const FHitResult Hit)
{
	ContactPoint = Hit.Location;
	ContacNormal = Hit.Normal;
}

void UTire::UpdateSuspension(const float Stiffness, const float Damping, const float SuspensionLength, const float TireStiffness, const float UnSprungMass, const float StaticTireLoad)
{
	SuspensionSettings.SpringStiffness = Stiffness;

	SuspensionSettings.DampingCoefficient = Damping;

	SuspensionSettings.SuspensionLength = SuspensionLength;

	SuspensionSettings.TireVerticalStiffness = TireStiffness;

	SuspensionSettings.UnSpringMass = UnSprungMass;

	//Get the static force on the string 
	UpdateTireLoad(StaticTireLoad);

	//Calculate the resting suspension length

	SuspensionSettings.RestPosition = SuspensionSettings.SpringStiffness != 0 ? StaticTireLoad / SuspensionSettings.SpringStiffness : 0;
}

UTire::~UTire()
{
}

float UTire::ClampToTireGrip(const float ThrottleForce) const
{
	return IsGrounded ?
		FMath::Clamp(ThrottleForce, -MaxGrip, MaxGrip)
		: 0;
}
