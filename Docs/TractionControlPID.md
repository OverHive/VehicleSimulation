# Traction Control via PID Controller

Implementation plan for `AVehicle` / `UTire` in VehicleSimulation. Since the front wheels are driven (`GetTireDriveForce` only drives `IsFrontTire`) and share one throttle value, a single controller watching the worst-case slip ratio is the right shape.

## 1. Reusable PID controller — new file `Source/VehicleSimulation/PIDController.h`

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PIDController.generated.h"

USTRUCT(BlueprintType)
struct FPIDController
{
	GENERATED_BODY()

	// Reacts to the current error
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	float ProportionalGain = 6.0f;

	// Removes steady-state error (keep small for traction control)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	float IntegralGain = 0.2f;

	// Damps the response (slip ratio is noisy, so keep this near zero)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	float DerivativeGain = 0.0f;

	// Anti-windup: bounds the accumulated integral
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	float IntegralClamp = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	float MinOutput = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	float MaxOutput = 1.0f;

	float Calculate(const float Error, const float DeltaTime)
	{
		if (DeltaTime <= 0.0f)
		{
			return FMath::Clamp(LastOutput, MinOutput, MaxOutput);
		}

		// Accumulate integral, clamped to prevent windup
		IntegralAccumulation += Error * DeltaTime;
		IntegralAccumulation = FMath::Clamp(IntegralAccumulation, -IntegralClamp, IntegralClamp);

		const float Derivative = (Error - PreviousError) / DeltaTime;
		PreviousError = Error;

		LastOutput = FMath::Clamp(
			ProportionalGain * Error
			+ IntegralGain * IntegralAccumulation
			+ DerivativeGain * Derivative,
			MinOutput, MaxOutput);

		return LastOutput;
	}

	void Reset()
	{
		IntegralAccumulation = 0.0f;
		PreviousError = 0.0f;
		LastOutput = 0.0f;
	}

private:
	float IntegralAccumulation = 0.0f;
	float PreviousError = 0.0f;
	float LastOutput = 0.0f;
};
```

## 2. Additions to `Vehicle.h`

```cpp
#include "PIDController.h"

// ... inside class AVehicle, public section:

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traction Control")
bool TractionControlEnabled = true;

// Slip ratio the controller holds — should match the peak of your Magic Formula curve
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traction Control")
float TargetSlipRatio = 0.12f;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traction Control")
FPIDController TractionPID;

// Runs the traction control loop once per tick
void UpdateTractionControl(float DeltaTime);

// ... private section:
float TractionThrottleModifier = 1.0f; // [0..1], applied to engine throttle
float TCSMaxSlipRatio = 0.0f;          // for debug display
```

## 3. Additions to `Vehicle.cpp`

### The control loop

```cpp
void AVehicle::UpdateTractionControl(float DeltaTime)
{
	// Bypass when there's nothing to control: no throttle, or below the speed
	// where your magic formula / slip ratio model is active (slip is meaningless at crawl speeds)
	if (!TractionControlEnabled || FMath::Abs(CurrentThrottle) < 0.05f || !IsUsingMagicFormula)
	{
		TractionThrottleModifier = 1.0f; // full throttle, no intervention
		TractionPID.Reset();
		return;
	}

	// The driven wheels share one throttle, so the worst spinner dictates the cut
	TCSMaxSlipRatio = 0.0f;
	for (UTire* Tire : FrontTires)
	{
		if (Tire)
		{
			TCSMaxSlipRatio = FMath::Max(TCSMaxSlipRatio, FMath::Abs(Tire->GetSlipRatio()));
		}
	}

	// Error > 0: grip to spare -> output saturates at 1 (no intervention)
	// Error < 0: wheel spinning past the tire curve's peak -> output drops, cutting throttle
	const float Error = TargetSlipRatio - TCSMaxSlipRatio;
	TractionThrottleModifier = TractionPID.Calculate(Error, DeltaTime);

	GEngine->AddOnScreenDebugMessage(27, 3.f, FColor::Yellow,
		FString::Printf(TEXT("TCS slip: %f / target %f -> throttle x%f"),
			TCSMaxSlipRatio, TargetSlipRatio, TractionThrottleModifier));
}
```

### Hook into `Tick`

Call it just before the tire loop (it uses last frame's slip ratio, which is fine at frame rate):

```cpp
CurrentDrivingForce = 0.0f;

UpdateTractionControl(DeltaTime); // <-- add here

for (int i = 0; i < AllTires.Num(); i++)
```

### Apply the cut in `GetDriveForce`

This is the single point where throttle becomes force, so both `GetTireDriveForce` and `UpdateWheel`'s call to `GetDriveForce(GetWheelTorque(Tire))` stay consistent automatically:

```cpp
float AVehicle::GetDriveForce(const float Torque) const
{
	return CurrentPresets.FrontWheelRadius > 0
		? Torque * CurrentThrottle * TractionThrottleModifier * GearRatio
			* CurrentPresets.DrivetrainEfficiency * CurrentPresets.FinalDriveRatio
			/ CurrentPresets.FrontWheelRadius
		: 0;
}
```

## How it maps to the simulation

| Piece | Role |
|---|---|
| `Tire->GetSlipRatio()` | Process variable — already computed in `UpdateWheel` |
| `TargetSlipRatio` | Setpoint — set it to the slip where `MagicFormula` peaks (trace it once at fixed load to find the peak) |
| `TractionThrottleModifier` | Actuator — a multiplier in `[0, 1]`, because traction control can only *cut* torque, never add beyond driver demand |
| `FrontTires` max slip | Feedback from the worst wheel, matching how an open differential delivers torque |

## Tuning guide

1. **Start with `IntegralGain = 0`, `DerivativeGain = 0`.** Raise `ProportionalGain` until wheel spin is caught quickly, but not so high that throttle "pumps" (oscillates). Sanity check on scale: a slip overshoot from 0.12 → 0.30 gives error −0.18, so `Kp ≈ 6` drives the modifier from 1.0 toward 0 in one step.
2. **Add `IntegralGain` (~0.1–0.5) only if** slip settles noticeably above/below target at steady state. The `IntegralClamp` matters here — while cruising below target slip, the integral winds up positive; the clamp keeps it from delaying the first intervention.
3. **Leave `DerivativeGain` at or near 0.** Slip ratio is computed from velocity ratios every frame, so it's noisy, and D amplifies that noise straight into throttle. If you see chattering, D is the culprit — the P term alone is usually enough for TCS.
4. **Tune live:** the gains are `EditAnywhere`, so they can be adjusted in PIE while watching the debug line (ID 27) respond.

## Edge case notes

- `Reset()` on bypass prevents a stale integral from cutting throttle the moment you get back on the power.
- Gating on `IsUsingMagicFormula` avoids fighting the controller at low speed, where `ClampToVehicleWheelSpeed` (not slip) governs the wheel.
- If per-wheel torque is added later (e.g., an electric motor per wheel or torque vectoring), switch to one `FPIDController` per driven wheel inside the tire loop — the struct is ready for that as-is.
