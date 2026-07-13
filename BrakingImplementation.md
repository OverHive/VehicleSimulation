# Wheel-Based Braking Implementation Guide

## Overview
This guide explains how to implement wheel-based braking in your vehicle simulation, replacing the current center-based braking with forces applied at each wheel's contact point.

## Current Implementation (Center-Based Braking)
The current braking system applies forces at the vehicle's center of mass:

```cpp
// Lines 114-118 in Vehicle.cpp
if (CurrentBrake > 0.0f)
{
    FVector BrakingForce = -MeshComponent->GetPhysicsLinearVelocity().GetSafeNormal() * (CurrentBrake * BrakeForce);
    MeshComponent->AddForce(BrakingForce, NAME_None, false);
}
```

## Recommended Wheel-Based Braking

Replace the current braking section with wheel-based braking inside your tire loop:

```cpp
for (int i = 0; i < AllTires.Num(); i++)
{
    UTire* Tire = AllTires[i];
    FVector SocketLocation = SkeletalMesh->GetSocketLocation(Tire->SocketName);
    
    if (Tire)
    {
        // Update the steering of the wheel
        Tire->UpdateSteering(CurrentSteeringAngle);
        
        // Get the direction of the wheel
        FVector WheelForward = Tire->GetForwardVector();
        
        // Update and apply the traction from the wheels
        Tire->UpdateTireLoad(LongitudinalAcceleration, LateralAcceleration);
        MeshComponent->AddForceAtLocation(Tire->GetTraction(CurrentThrottle * ThrottleForce) * WheelForward, SocketLocation);
        
        // === WHEEL-BASED BRAKING ===
        if (CurrentBrake > 0.0f && Tire->IsGrounded)
        {
            // Get wheel velocity direction
            FVector WheelVelocity = MeshComponent->GetPhysicsLinearVelocityAtPoint(SocketLocation);
            FVector BrakeDirection = -WheelVelocity.GetSafeNormal();
            
            // Calculate braking force based on tire load (more load = more braking)
            float TireLoadRatio = Tire->TireLoad / (VehicleMass * 981.0f / 4.0f); // Normalize per wheel
            float WheelBrakeForce = CurrentBrake * BrakeForce * TireLoadRatio;
            
            // Apply braking at the wheel's contact point
            FVector BrakingForce = BrakeDirection * WheelBrakeForce;
            MeshComponent->AddForceAtLocation(BrakingForce, Tire->ContactPoint);
        }
        // === END WHEEL-BASED BRAKING ===
        
        // Get wheel right vector for lateral calculations
        FVector WheelRight = FVector::CrossProduct(MeshComponent->GetUpVector(), WheelForward);
        
        // Get the wheel's velocity
        FVector VelocityAtWheel = MeshComponent->GetPhysicsLinearVelocityAtPoint(SocketLocation);
        
        // Calculate the lateral speed
        float LateralSpeed = FVector::DotProduct(VelocityAtWheel, WheelRight);
        
        // Force opposing sideways slip, capped by the tire's grip
        float LateralStiffness = 5.0f;
        float DesiredForce = -LateralSpeed * LateralStiffness;
        
        float MaxGrip = Tire->GetLateralGrip();
        FVector LateralFriction = WheelRight * FMath::Clamp(DesiredForce, -MaxGrip, MaxGrip);
        
        MeshComponent->AddForceAtLocation(LateralFriction, Tire->ContactPoint);
        
        // Debug output
        float TireLoad = Tire->TireLoad;
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(4 + i, 3.f, FColor::Green, FString::Printf(TEXT("%s's tire load: %f N"), *Tire->GetName(), TireLoad));
    }
}
```

## Key Improvements

### 1. Braking at Contact Points
Forces are applied at each wheel's contact point instead of the vehicle center, creating more realistic weight transfer during braking.

### 2. Load-Based Braking
Braking force is distributed based on tire load - wheels with more load get more braking power, mimicking real vehicle behavior.

### 3. Grounded Check
Only applies braking to wheels that are in contact with the ground.

### 4. Wheel Velocity Direction
Uses the actual velocity direction at each wheel for more accurate braking vectors.

## Optional Enhancements

Add these parameters to your `Vehicle.h` file for more control over braking behavior:

```cpp
// Add to Vehicle.h in the Physics section
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
float FrontBrakeBias = 0.7f;  // 70% front brake bias (default)

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")  
float MaxBrakeForcePerWheel = 75000.0f;  // Per-wheel maximum brake force
```

## Advanced Brake Bias Implementation

If you add the brake bias parameters, you can enhance the braking code like this:

```cpp
// Enhanced braking with brake bias
if (CurrentBrake > 0.0f && Tire->IsGrounded)
{
    // Get wheel velocity direction
    FVector WheelVelocity = MeshComponent->GetPhysicsLinearVelocityAtPoint(SocketLocation);
    FVector BrakeDirection = -WheelVelocity.GetSafeNormal();
    
    // Calculate braking force based on tire load
    float TireLoadRatio = Tire->TireLoad / (VehicleMass * 981.0f / 4.0f);
    
    // Apply brake bias (front wheels get more braking power)
    float BrakeBias = Tire->IsFrontTire ? FrontBrakeBias : (1.0f - FrontBrakeBias);
    float NormalizedBrakeBias = BrakeBias / (Tire->IsFrontTire ? 2.0f : 2.0f); // Normalize for front/rear count
    
    float WheelBrakeForce = CurrentBrake * BrakeForce * TireLoadRatio * NormalizedBrakeBias;
    
    // Clamp to maximum per-wheel brake force
    WheelBrakeForce = FMath::Min(WheelBrakeForce, MaxBrakeForcePerWheel);
    
    // Apply braking at the wheel's contact point
    FVector BrakingForce = BrakeDirection * WheelBrakeForce;
    MeshComponent->AddForceAtLocation(BrakingForce, Tire->ContactPoint);
}
```

## Physical Benefits

### Weight Transfer
The wheel-based approach creates realistic forward weight transfer (nosedive) during hard braking, as the braking forces applied at the contact points create torque that rotates the vehicle forward.

### Stability
Load-based braking ensures that wheels with more traction (more load) do more braking work, improving overall vehicle stability.

### Realistic Behavior
This approach mimics how real vehicles brake, where brake pads clamp onto rotors at each wheel, creating stopping forces at the contact patches.

## Implementation Steps

1. **Remove current center-based braking**: Delete or comment out lines 114-118 in Vehicle.cpp
2. **Add wheel-based braking**: Insert the new braking code inside your tire loop, after traction application
3. **Test**: Test the braking behavior and adjust BrakeForce as needed
4. **Optional enhancements**: Add brake bias parameters for more control

## Notes

- The current implementation assumes 4 wheels, adjust the normalization factor if you have different wheel counts
- Brake bias typically favors front wheels (60-70% front) because front wheels gain more load during braking
- You may need to reduce the overall BrakeForce value when switching to wheel-based braking, as the total force will be higher
- Ensure your tires have proper ContactPoint values set in your suspension raycast system