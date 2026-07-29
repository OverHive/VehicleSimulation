# Adding Unsprung Masses to Half-Car Model

## Concept Overview

**Current Model**: Sprung mass (vehicle body) only - 2 degrees of freedom (heave + pitch)

**With Unsprung Masses**: Sprung mass + 2 unsprung masses (front/rear axles) - 4 degrees of freedom

### What are Unsprung Masses?
Unsprung masses are the portions of the vehicle that are not supported by the suspension:
- Wheels/tires
- Brake assemblies
- Axles/half-shafts
- Part of the suspension components (control arms, etc.)

## Implementation Steps

### 1. Add Unsprung Mass Variables (in Vehicle.h)

```cpp
// Unsprung mass properties
UPROPERTY(EditAnywhere, Category = "Suspension")
float FrontUnsprungMass = 50.0f;  // kg (front axle + wheels)

UPROPERTY(EditAnywhere, Category = "Suspension")
float RearUnsprungMass = 50.0f;   // kg (rear axle + wheels)

// Unsprung mass state (vertical dynamics)
float FrontUnsprungPosition = 0.0f;
float FrontUnsprungVelocity = 0.0f;
float RearUnsprungPosition = 0.0f;
float RearUnsprungVelocity = 0.0f;

// Forces for calculation
float FrontUnspringForce = 0.0f;
float RearUnspringForce = 0.0f;

// Tire vertical stiffness
UPROPERTY(EditAnywhere, Category = "Tire")
float TireVerticalStiffness = 200000.0f;  // N/m
```

### 2. Modify Suspension Force Calculation

Instead of applying suspension forces directly to the sprung mass, calculate them as forces between sprung and unsprung masses:

```cpp
void AVehicle::CalculateSuspensionDynamics(float DeltaTime, float LongitudinalAcceleration, const float LateralAcceleration)
{
    CalculatePitchWeightTransfer(LongitudinalAcceleration, LateralAcceleration);
    SuspensionRayCast();
    
    // NEW: Calculate unsprung mass dynamics
    CalculateUnsprungMassDynamics(DeltaTime);
    
    CalculatePitchAndHeaveDynamics(DeltaTime, LongitudinalAcceleration);
    ApplySuspensionForceEffects();
}
```

### 3. Add Unsprung Mass Dynamics Function

```cpp
void AVehicle::CalculateUnsprungMassDynamics(float DeltaTime)
{
    FrontUnspringForce = 0.0f;
    RearUnspringForce = 0.0f;
    
    for (UTire*& Tire : AllTires)
    {
        if (!Tire->IsGrounded) continue;
        
        float UnsprungMass = Tire->IsFrontTire ? FrontUnsprungMass : RearUnsprungMass;
        
        // Get suspension force (spring + damper)
        float SuspensionForce = Tire->GetSuspensionForce();
        
        // Get tire vertical force (from tire contact patch)
        float TireForce = Tire->TireLoad;
        
        // Net force on unsprung mass = Tire force - Suspension force
        float NetUnsprungForce = TireForce - SuspensionForce;
        
        // Update unsprung mass dynamics
        float UnsprungAcceleration = NetUnsprungForce / UnsprungMass;
        
        if (Tire->IsFrontTire)
        {
            FrontUnsprungVelocity += UnsprungAcceleration * DeltaTime;
            FrontUnsprungPosition += FrontUnsprungVelocity * DeltaTime;
            FrontUnspringForce = SuspensionForce; // Store for sprung mass calculation
        }
        else
        {
            RearUnsprungVelocity += UnsprungAcceleration * DeltaTime;
            RearUnsprungPosition += RearUnsprungVelocity * DeltaTime;
            RearUnspringForce = SuspensionForce;
        }
        
        // Add damping to unsprung mass
        if (Tire->IsFrontTire)
            FrontUnsprungVelocity *= 0.95f;
        else
            RearUnsprungVelocity *= 0.95f;
    }
}
```

### 4. Update Pitch and Heave Calculations

Modify `CalculatePitchAndHeaveDynamics` to use forces from unsprung masses:

```cpp
void AVehicle::CalculatePitchAndHeaveDynamics(float DeltaTime, float LongitudinalAcceleration)
{
    // Use forces acting on sprung mass from unsprung masses
    float PitchMoment = (RearUnspringForce * DistanceOfCentreOfGravityToRearAxis) - 
                       (FrontUnspringForce * DistanceOfCentreOfGravityToFrontAxis);
    
    float WeightTransferMoment = VehicleMass * LongitudinalAcceleration * CentreOfGravityHeight;
    WeightTransferMoment *= AntiDiveFactor;
    PitchMoment += WeightTransferMoment;
    
    // Get the pitch acceleration from the moment
    float PitchAcceleration = PitchInertia != 0.0f ? PitchMoment / PitchInertia : 0;
    
    // Update the pitch velocity and angle
    PitchVelocity += PitchAcceleration * DeltaTime;
    PitchAngle += PitchVelocity * DeltaTime;
    
    // Add pitch damping to prevent oscillation
    PitchVelocity *= PitchDamping;
    
    // Clamp pitch angle to realistic limits
    PitchAngle = FMath::Clamp(PitchAngle, FMath::DegreesToRadians(-15.0f), FMath::DegreesToRadians(15.0f));
    
    // Get the overall vertical force on the sprung mass
    float TotalSuspensionForce = FrontUnspringForce + RearUnspringForce;
    float VehicleWeight = VehicleMass * Gravity;
    float NetVerticalForce = TotalSuspensionForce - VehicleWeight * Gravity;
    
    // Calculate the heave acceleration with acceleration = force/mass
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
```

### 5. Add Tire Vertical Stiffness Calculation

Unsprung masses interact with the ground through tire stiffness:

```cpp
// In Tire.cpp or as part of suspension calculation
float TireDeflection = FMath::Max(0.0f, WheelRadius - GroundDistance);
float TireVerticalForce = TireDeflection * TireVerticalStiffness;
```

## Physical Model Summary

### Degrees of Freedom
- **Sprung Mass**: Heave (vertical) + Pitch (rotational)
- **Front Unsprung Mass**: Vertical motion
- **Rear Unsprung Mass**: Vertical motion

### Force Flow
1. **Ground → Tire**: Tire vertical force (tire stiffness)
2. **Tire → Unsprung Mass**: Tire force acts upward
3. **Unsprung Mass → Suspension**: Suspension force acts downward
4. **Suspension → Sprung Mass**: Equal and opposite suspension force

### Key Equations
```
For Unsprung Mass:
  F_net = F_tire - F_suspension
  a = F_net / m_unsprung
  v_new = v_old + a * dt
  x_new = x_old + v_new * dt

For Sprung Mass:
  F_pitch = (F_rear_suspension * rear_distance) - (F_front_suspension * front_distance)
  α = F_pitch / I_pitch
  ω_new = ω_old + α * dt
  θ_new = θ_old + ω_new * dt
```

## Benefits

1. **More Realistic High-Frequency Behavior**: Captures wheel hop, vibration, and road irregularities
2. **Better Tire Load Calculation**: Accounts for unsprung mass inertia effects
3. **Improved Ride Quality Simulation**: Models sprung/unsprung interaction accurately
4. **More Accurate Weight Transfer**: Includes unsprung mass effects during dynamic maneuvers
5. **Better Handling Characteristics**: Unsprung mass affects vehicle response to inputs

## Physical Considerations

### Typical Values
- **Unsprung mass ratio**: 10-15% of total vehicle mass
- **Front unsprung mass**: Usually higher (struts, brakes, drive components)
- **Rear unsprung mass**: Often lighter (beam axle vs independent suspension)

### Stiffness Comparison
- **Suspension stiffness**: 20-60 N/mm (spring rate)
- **Tire vertical stiffness**: 150-300 N/mm (much stiffer than suspension)
- **Tire acts as secondary spring** in series with suspension

### Damping
- **Suspension damping**: Controls sprung mass motion
- **Tire damping**: Minimal (tires have very little internal damping)
- **Unsprung mass damping**: Needs artificial damping for simulation stability

## Testing Recommendations

1. **Start with typical values**: Front 50-60kg, Rear 40-50kg
2. **Tire stiffness**: Begin with 200,000 N/m
3. **Monitor stability**: Unsprung masses can cause high-frequency oscillations
4. **Adjust damping**: Increase unsprung mass damping if simulation becomes unstable
5. **Validate behavior**: Check that wheel hop and tire load variations look realistic

## This implementation creates a **4-degree-of-freedom model** vs your current 2-DOF model, significantly improving simulation fidelity for road irregularities and high-frequency dynamics.
