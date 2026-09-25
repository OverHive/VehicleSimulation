// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleHUD.h"

VehicleHUD::VehicleHUD()
{
}

void VehicleHUD::BodyHUD(float ForwardVelocity, float LongitudinalAcceleration, float CurrentThrottle, float CurrentBrake,
	float CurrentSteeringAngle, float CurrentSteering, int GearIndex, float CurrentDrag,
	float GearRatio, float FrontAxleShare, float RearAxleShare, float VehicleMass, float VerticalVelocity, float VehicleWeight,
	int VehiclePresetIndex, FName VehiclePresetName, float CurrentDrivingForce, float EngineCapType)
{
	if (GEngine)
	{
		FString VehicleBlock = FString::Printf(TEXT("=== VEHICLE =======================================\n")
			TEXT("Speed %5.1f km/h | Accel %5.2f m/s^2\n")
			TEXT("Thr %+0.2f | Brk %0.2f | Steer %+0.2f (%+0.1f deg) | Gear %d (%0.2f)\n")
			TEXT("Drag %0.1f N | Weight %0.1f N | Axle load F/R %0.1f/%0.1f %%\n")
			TEXT("Body mass %0.1f kg | Vert vel %+0.1f cm/s"),
			ForwardVelocity*0.036f,
			LongitudinalAcceleration * 0.01f,
			CurrentThrottle, CurrentBrake, CurrentSteering, CurrentSteeringAngle, GearIndex + 1, GearRatio, //1-based for display - 016 acceptance c5 (change 023)
			CurrentDrag / 100.0f, VehicleWeight / 100.0f, 100 * FrontAxleShare, 100 * RearAxleShare,
			VehicleMass, VerticalVelocity);
		VehicleBlock.Appendf(TEXT("\nPreset %d: "), VehiclePresetIndex);
		VehicleBlock.Append(VehiclePresetName.ToString());
		VehicleBlock.Appendf(TEXT("\nEng drive %0.1f N cap "), CurrentDrivingForce / 100.0f);
		const FString CapName = EngineCapType == 0 ? FString(TEXT("torque")) : (EngineCapType == 1 ? FString(TEXT("power")) : FString(TEXT("off")));
		VehicleBlock.Append(CapName);
		GEngine->AddOnScreenDebugMessage(101, 3.f, FColor::Green, VehicleBlock);
	}
}

void VehicleHUD::WheelHUD(TArray<UTire*> Tires)
{
	//Key 102 - per-wheel table.
	if (GEngine)
	{
		const TCHAR* WheelLabels[4] = { TEXT("FR"), TEXT("FL"), TEXT("RR"), TEXT("RL") };
		FString WheelsBlock = FString(TEXT("=== WHEELS: Grd Comp(cm) Load(N) SusF(N) Trac(N) Grip% Slip angle(deg) Slip ratio ==="));
		for (int i = 0; i < Tires.Num(); i++)
		{
			UTire* Tire = Tires[i];
			if (!Tire)
			{
				continue;
			}
			const TCHAR* Label = i < 4 ? WheelLabels[i] : TEXT("??");
			//Fill in the data for ground wheels
			if (Tire->IsGrounded)
			{
				//Get the traction for the wheel
				const float Traction = Tire->GetLastLongitudinalForce();
				//Get the lateral friction for the wheel
				const float LateralFriction = Tire->GetLastLateralForce();
				//Get the combined slip friction for the wheel
				const float CombinedSlip = FMath::Sqrt(FMath::Pow(Traction, 2) + FMath::Pow(LateralFriction, 2));
				//Calculate the grip remaining
				const float GripPercent = Tire->GetMaxGrip() > 0.0f ? CombinedSlip / Tire->GetMaxGrip() * 100.0f : 0.0f;
				WheelsBlock.Appendf(TEXT("\n%s  Y %5.1f %7.1f %7.1f %+7.1f %5.1f%% %6.1f,%6.1f"),
					Label,
					Tire->GetCompression(),
					Tire->TireLoad / 100.0f,
					Tire->GetSuspensionForce() / 100.0f,
					Traction / 100.0f,
					GripPercent,
					Tire->GetSlipAngle(),
					Tire->GetSlipRatio()
					);
			}
			//Skip over aerial wheels
			else
			{
				WheelsBlock.Appendf(TEXT("\n%s  N %5s %7s %7s %7s %5s %6s"),
					Label, TEXT("-"), TEXT("-"), TEXT("-"), TEXT("-"), TEXT("-"), TEXT("-"));
			}
		}
		GEngine->AddOnScreenDebugMessage(102, 3.f, FColor::Yellow, WheelsBlock);
	}
}

void VehicleHUD::SuspensionHUD(TArray<UTire*> Tires, float PitchAngle, float HeavePosition)
{
	//Key 103 - suspension state, per-axle averages of ray distance vs rest position.
	if (GEngine)
	{
		float FrontDistanceSum = 0.0f;
		float FrontRestSum = 0.0f;
		float RearDistanceSum = 0.0f;
		float RearRestSum = 0.0f;
		int FrontWheelCount = 0;
		int RearWheelCount = 0;
		//Get the distance the in the axes are from rest  
		for (UTire* Tire : Tires)
		{
			if (Tire && Tire->IsGrounded)
			{
				if (!Tire->WheelConfig.IsRearWheel)
				{
					FrontDistanceSum += Tire->GetCompression();
					FrontRestSum += Tire->SuspensionSettings.RestPosition;
					FrontWheelCount++;
				}
				else
				{
					RearDistanceSum += Tire->GetCompression();
					RearRestSum += Tire->SuspensionSettings.RestPosition;
					RearWheelCount++;
				}
			}
		}
		const float FrontDistance = FrontWheelCount > 0 ? FrontDistanceSum / FrontWheelCount : 0.0f;
		const float FrontRest = FrontWheelCount > 0 ? FrontRestSum / FrontWheelCount : 0.0f;
		const float RearDistance = RearWheelCount > 0 ? RearDistanceSum / RearWheelCount : 0.0f;
		const float RearRest = RearWheelCount > 0 ? RearRestSum / RearWheelCount : 0.0f;
		//Dash out any axes without out grounded wheels
		const FString FrontAxle = FrontWheelCount > 0 ? FString::Printf(TEXT("%0.1f/%0.1f"), FrontDistance, FrontRest) : FString(TEXT("-"));
		const FString RearAxle = RearWheelCount > 0 ? FString::Printf(TEXT("%0.1f/%0.1f"), RearDistance, RearRest) : FString(TEXT("-"));

		FString SuspensionBlock = FString::Printf(TEXT("=== SUSPENSION ===================================\n")
			TEXT("Pitch(ode-pred) %+0.2f deg | Heave(ode-pred) %+0.2f cm\n"),
			FMath::RadiansToDegrees(PitchAngle), HeavePosition);
		SuspensionBlock.Appendf(TEXT("Dist/rest F %s | R %s cm"), *FrontAxle, *RearAxle);
		GEngine->AddOnScreenDebugMessage(103, 3.f, FColor::Cyan, SuspensionBlock);
	}
}

VehicleHUD::~VehicleHUD()
{
}
