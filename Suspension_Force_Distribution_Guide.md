# Suspension Force Distribution Based on Axis and Wheelbase

This document explains how to redistribute suspension forces geometrically across tires based on vehicle geometry rather than having each tire's force determined independently by its spring compression.

## Current Problem

Currently, each tire's normal force is calculated independently:
```cpp
float SpringForceMagnitude = Compression * Wheel.SpringStiffness;
```

This means:
- Forces are uneven and terrain-dependent
- No guarantee forces sum to total vehicle weight
- Unpredictable weight distribution

## Solution: Geometric Distribution

The approach is to:
1. Calculate total suspension force from all springs
2. Distribute that total force based on vehicle geometry (wheelbase, track width, CG position)
3. Apply distributed forces at each tire's contact point

---

## Implementation Approach

### 1. Calculate Total Suspension Force First

Instead of applying spring forces immediately, accumulate them:

```cpp
// Phase 1: Detect Contact and Calculate Total Force
float TotalSpringForce = 0.0f;
float TotalDampingForce = 0.0f;
int TiresOnGround = 0;

for (UTire* Tire : AllTires) {
    // Raycast to detect contact
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params)) {
        // Calculate compression and velocity
        float Compression = Wheel.SuspensionLength - (CurrentDistance - Wheel.WheelRadius);
        Compression = FMath::Max(0.0f, Compression);
        
        FVector VelocityAtPoint = MeshComponent->GetPhysicsLinearVelocityAtPoint(StartLocation);
        float SuspensionVelocity = FVector::DotProduct(VelocityAtPoint, VehicleUpDirection);
        
        // Accumulate forces (don't apply yet)
        TotalSpringForce += Compression * Tire->SuspensionSettings.SpringStiffness;
        TotalDampingForce += SuspensionVelocity * Tire->SuspensionSettings.DampingCoefficient;
        
        TiresOnGround++;
        Tire->IsGrounded = true;
        Tire->ContactPoint = Hit.Location;
    } else {
        Tire->IsGrounded = false;
    }
}

float TotalSuspensionForce = TotalSpringForce - TotalDampingForce;
```

### 2. Distribute Total Force Geometrically

For each tire, calculate its share based on position:

```cpp
float CalculateGeometricShare(const UTire* Tire, float TotalForce, const AVehicle* Vehicle) {
    // Static distribution based on CG position
    float FrontRearRatio;
    if (Tire->IsFrontTire) {
        FrontRearRatio = Vehicle->DistanceOfCentreOfGravityToRearAxis / Vehicle->WheelBaseLength;
    } else {
        FrontRearRatio = Vehicle->DistanceOfCentreOfGravityToFrontAxis / Vehicle->WheelBaseLength;
    }
    
    // Divide by 2 for left/right sharing on same axle
    float StaticShare = (TotalForce * FrontRearRatio) / 2.0f;
    
    // Calculate current accelerations for dynamic load transfer
    FVector CurrentVelocity = Vehicle->MeshComponent->GetPhysicsLinearVelocity();
    FVector Acceleration = (CurrentVelocity - Vehicle->LastVelocity) / DeltaTime;
    float LongitudinalAccel = FVector::DotProduct(Acceleration, Vehicle->GetActorForwardVector());
    float LateralAccel = FVector::DotProduct(Acceleration, Vehicle->GetActorRightVector());
    
    // Longitudinal weight transfer (front gains under braking, rear gains under acceleration)
    float LongitudinalTransfer = LongitudinalAccel * Vehicle->VehicleMass * 
        (Vehicle->CentreOfGravityHeight / Vehicle->WheelBaseLength) * 
        (Tire->IsFrontTire ? -1.0f : 1.0f) / 2.0f;
    
    // Lateral weight transfer (outer tires gain in corners)
    float LateralTransfer = LateralAccel * Vehicle->VehicleMass * 
        (Vehicle->CentreOfGravityHeight / Vehicle->TrackWidth) * 
        (Tire->IsRightTire ? 1.0f : -1.0f) / 2.0f;
    
    float DistributedForce = StaticShare + LongitudinalTransfer + LateralTransfer;
    return FMath::Max(0.0f, DistributedForce);
}
```

### 3. Apply Distributed Forces to Contact Points

Replace individual spring forces with geometrically distributed ones:

```cpp
// Phase 2: Distribute and Apply Forces
for (UTire* Tire : AllTires) {
    if (Tire->IsGrounded) {
        float TireNormalForce = CalculateGeometricShare(Tire, TotalSuspensionForce, this);
        
        FVector DistributedUpForce = VehicleUpDirection * TireNormalForce;
        FVector ContactPoint = Tire->ContactPoint;
        
        // Apply the distributed force at the tire's contact point
        MeshComponent->AddForceAtLocation(DistributedUpForce, ContactPoint);
        
        // Update the tire's normal force with the distributed value
        Tire->UpdateWheelSuspension(DistributedUpForce, ContactPoint);
        Tire->TireLoad = TireNormalForce;
    }
}
```

---

## Alternative: Average Compression Method

For a more natural suspension feel with geometric distribution:

```cpp
// Calculate average compression across all grounded tires
float TotalCompression = 0.0f;
float TotalVelocity = 0.0f;
int GroundedTires = 0;

for (UTire* Tire : AllTires) {
    if (Tire->IsGrounded) {
        TotalCompression += Tire->CurrentCompression;
        TotalVelocity += Tire->SuspensionVelocity;
        GroundedTires++;
    }
}

float AverageCompression = GroundedTires > 0 ? TotalCompression / GroundedTires : 0.0f;
float AverageVelocity = GroundedTires > 0 ? TotalVelocity / GroundedTires : 0.0f;

// Use average compression with average stiffness values
float AverageSpringStiffness = /* calculate from all tires */;
float AverageDampingCoefficient = /* calculate from all tires */;

float BaseSpringForce = AverageCompression * AverageSpringStiffness;
float BaseDampingForce = AverageVelocity * AverageDampingCoefficient;
float TotalSuspensionForce = BaseSpringForce - BaseDampingForce;

// Then distribute geometrically as shown above
```

---

## What This Changes

### Before (Current Approach):
```
Each tire's normal force = its own spring compression × its own stiffness
```
- Uneven, terrain-dependent forces
- No guarantee forces sum correctly
- Unpredictable weight distribution

### After (Geometric Distribution):
```
Each tire's normal force = (total suspension force) × (geometric share)
```
- Predictable, geometry-based distribution
- Forces always distributed correctly
- Responds to overall suspension compression

---

## What the Suspension Still Does

The suspension mechanics still:
- ✅ Detects ground contact via raycast
- ✅ Calculates compression and velocity
- ✅ Responds to road surface (bumps, dips)
- ✅ Provides natural ride height and damping
- ✅ Creates realistic suspension movement

But now the forces are:
- ✅ Summed across all tires
- ✅ Redistributed based on vehicle geometry
- ✅ Applied predictably according to wheelbase and track width
- ✅ Match the theoretical weight distribution used in grip calculations

---

## Key Implementation Steps

1. **Restructure `SuspensionRayCast()`** into two phases:
   - Phase 1: Accumulate total spring/damper forces
   - Phase 2: Distribute and apply forces geometrically

2. **Add `CalculateGeometricShare()` function** to compute each tire's share based on:
   - Static weight distribution (CG position relative to axles)
   - Longitudinal weight transfer (acceleration/braking)
   - Lateral weight transfer (cornering)

3. **Update `Tire::UpdateWheelSuspension()`** to store the distributed force instead of the raw spring force

4. **Synchronize `TireLoad`** with the distributed normal force so grip calculations match the actual forces being applied

---

## Expected Benefits

1. **Consistent Weight Distribution**: Forces always follow geometric rules
2. **Predictable Handling**: Same inputs produce same weight transfer
3. **Better Tire Grip Integration**: Tire load matches the force actually applied
4. **More Realistic Behavior**: Matches how real vehicle suspensions distribute forces
5. **Smoother Transitions**: Weight transfer is continuous, not jumpy

---

## Files to Modify

- `Vehicle.cpp`: 
  - `SuspensionRayCast()` method (lines 208-281)
  - Add geometric distribution calculation
  
- `Tire.cpp`:
  - `UpdateWheelSuspension()` method (lines 133-137)
  - Ensure `TireLoad` is updated with distributed force

---

## Notes

- This approach keeps the natural feel of suspension springs while ensuring forces are distributed geometrically
- The total suspension force will naturally match the vehicle's weight due to physics equilibrium
- Dynamic load transfer from acceleration/braking/cornering is automatically included
- Forces can be clamped to zero to prevent tires pulling the ground (no negative normal forces)
