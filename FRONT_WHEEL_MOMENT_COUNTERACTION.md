# Front Wheel Moment Counteraction Under Throttle

## Current Problem Analysis

When throttle is applied, the system currently:
1. **Applies traction force only to front tires** (Vehicle.cpp:191, Tire.cpp:154-157)
2. **Creates a nose-up pitching moment** because the forward force is applied at the front contact patches, which are ahead of the center of mass
3. The moment = Force × Distance from CG to front axle

The moment arm from front axle to CG is defined by `DistanceOfCentreOfGravityToFrontAxis = 59.0cm` (Vehicle.h:78).

## Existing Counteraction Mechanisms

The code already has some mechanisms in place:

### 1. Anti-Squat System (Vehicle.cpp:148, 150-151)
```cpp
AntiDiveFactor = 1.0f - (IsBraking ? AntiDivePercentage : AntiSquatPercentage);
```
- `AntiSquatPercentage = 0.25f` reduces weight transfer effects under acceleration
- Modifies `AntiDiveFactor` which affects both braking and acceleration dynamics
- Applied in weight transfer calculations (Vehicle.cpp:426, 482)

### 2. Pitch Dynamics System (Vehicle.cpp:455-513)
- Calculates pitch moments from suspension force imbalances
- Computes weight transfer moments: `VehicleMass * LongitudinalAcceleration * CentreOfGravityHeight`
- Applies pitch damping to prevent oscillation
- Updates pitch angle and velocity based on suspension forces

### 3. Angular Damping System (Vehicle.cpp:115-138)
- Currently only active when steering is released
- Applies counter-torque: `MeshComponent->AddTorqueInRadians(CounterTorque)`
- Could be extended for pitch-axis damping under throttle

## Counteraction Mechanisms (Excluding RWD)

### 1. Active Suspension Management

**Increase rear suspension force under throttle**
- Location: Vehicle.cpp:464-474 (in `CalculatePitchAndHeaveDynamics`)
- Mechanism: Dynamically increase rear suspension spring force when throttle is applied
- Effect: Transfers load from front to rear, reducing front tire normal force and the resulting moment

**Implementation approach:**
```cpp
// In CalculatePitchAndHeaveDynamics, modify rear suspension force
if (!IsBraking && CurrentThrottle > 0.0f) {
    RearSuspensionForce *= (1.0f + AntiSquatPercentage);
}
```

### 2. Direct Counter-Torque Application

**Apply pitch-resisting torque directly to the physics body**
- Use `MeshComponent->AddTorqueInRadians()` to apply counter-torque around the pitch axis
- Calculate required counter-torque based on:
  - Throttle force magnitude
  - Distance from CG to front axle (`DistanceOfCentreOfGravityToFrontAxis`)
  - Current pitch velocity for damping

**Implementation approach:**
```cpp
// In Tick function, after throttle force application
if (CurrentThrottle > 0.0f) {
    // Calculate moment arm to front wheels
    float FrontMomentArm = DistanceOfCentreOfGravityToFrontAxis;
    
    // Calculate the counteracting torque needed
    float ThrottleMoment = CurrentThrottle * ThrottleForce * FrontMomentArm;
    
    // Apply counter-torque (negative pitch torque to resist nose-up)
    FVector PitchCounterTorque = GetActorRightVector() * (-ThrottleMoment * AntiSquatFactor);
    MeshComponent->AddTorqueInRadians(PitchCounterTorque);
}
```

### 3. Enhanced Weight Transfer Compensation

**Increase AntiSquatPercentage for stronger compensation**
- Current value: `AntiSquatPercentage = 0.25f` (Vehicle.h:151)
- This parameter reduces the effective weight transfer under acceleration
- Higher values provide more pitch resistance

**Current implementation:**
```cpp
// Vehicle.cpp:426, 482
LongitudinalWeightTransfer *= AntiDiveFactor;
WeightTransferMoment *= AntiDiveFactor;
```

**Enhancement approach:**
- Make `AntiSquatPercentage` throttle-dependent
- Increase anti-squat effect at higher throttle values
- Add additional compensation specifically for front wheel moments

### 4. Suspension Force Redistribution

**Dynamic stiffness adjustment based on throttle state**
- Temporarily increase rear spring stiffness under throttle
- Temporarily decrease front spring stiffness under throttle
- Shifts load distribution rearward without changing total vehicle load

**Implementation approach:**
```cpp
// Modify suspension stiffness dynamically
float FrontStiffnessModifier = 1.0f - (CurrentThrottle * 0.2f);  // Reduce by up to 20%
float RearStiffnessModifier = 1.0f + (CurrentThrottle * 0.3f);   // Increase by up to 30%

// Apply modified stiffness in suspension force calculation
FrontSuspensionForce *= FrontStiffnessModifier;
RearSuspensionForce *= RearStiffnessModifier;
```

### 5. Pitch-Axis Angular Velocity Damping

**Extend existing angular damping system to pitch axis**
- Currently the steering release damping (Vehicle.cpp:115-138) only affects yaw (Z-axis)
- Can be extended to pitch axis (around local right vector)
- Provides direct resistance to pitch rotation induced by front wheel moments

**Implementation approach:**
```cpp
// Get pitch angular velocity (around right vector)
FVector AngularVelocity = MeshComponent->GetPhysicsAngularVelocityInRadians();
float PitchAngularVelocity = FVector::DotProduct(AngularVelocity, GetActorRightVector());

// Calculate pitch damping coefficient
float SpeedFactor = FMath::Clamp(CurrentVelocity.Size() / 1000.0f, 0.1f, 1.0f);
float PitchDampingCoefficient = PitchDamping * VehicleMass * SpeedFactor;

// Apply counter-torque to resist pitch
FVector PitchCounterTorque = GetActorRightVector() * (-PitchAngularVelocity * PitchDampingCoefficient);
MeshComponent->AddTorqueInRadians(PitchCounterTorque);
```

## Recommended Implementation Strategy

**Most effective approach: Combine multiple mechanisms**

1. **Primary:** Direct counter-torque application (Mechanism #2)
   - Most direct way to counteract the moment
   - Calculated based on actual throttle force and geometry

2. **Secondary:** Enhanced anti-squat compensation (Mechanism #3)
   - Leverages existing `AntiSquatPercentage` system
   - Already integrated into weight transfer calculations

3. **Supporting:** Pitch-axis damping (Mechanism #5)
   - Prevents oscillation and overshoot
   - Similar to existing steering damping system

**Implementation priority:**
1. Extend anti-squat to be more aggressive under high throttle
2. Add direct counter-torque calculation based on throttle force
3. Implement pitch-axis velocity damping
4. Add dynamic suspension stiffness adjustment if needed

The key advantage of this multi-faceted approach is that it addresses the problem at multiple levels:
- **Counter-torque** directly opposes the moment
- **Anti-squat** reduces the weight transfer that contributes to pitching
- **Damping** prevents oscillation and provides stability

## Key Parameters for Tuning

- `AntiSquatPercentage` (currently 0.25) - increase for stronger pitch resistance
- `PitchDamping` (currently 0.95) - adjust pitch velocity decay rate
- `DistanceOfCentreOfGravityToFrontAxis` (59.0cm) - moment arm for counter-torque calculation
- `ThrottleForce` (1500000.0 cN) - magnitude of force creating the moment

## Testing and Validation

**Verify effectiveness by monitoring:**
- Pitch angle and velocity under steady throttle
- Front vs rear tire load distribution during acceleration
- Vehicle stability and handling characteristics
- Oscillation and overshoot behavior

**Test scenarios:**
- Gradual throttle application
- Sudden throttle input (tip-in)
- Various throttle levels (partial to full)
- Different vehicle speeds