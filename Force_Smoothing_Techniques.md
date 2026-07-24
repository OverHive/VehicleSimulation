# Force Smoothing Techniques for Vehicle Simulation

This document outlines techniques for smoothly applying forces in your vehicle simulation to prevent pitching, instability, and physics artifacts.

## Table of Contents
1. [Brake Force Interpolation](#1-brake-force-interpolation-most-important)
2. [Suspension Force Smoothing](#2-suspension-force-smoothing)
3. [Weight Transfer Smoothing](#3-weight-transfer-smoothing)
4. [Force Rate Limiting](#4-force-rate-limiting)
5. [Exponential Moving Average](#5-exponential-moving-average)
6. [Wheel Contact Transition Smoothing](#6-wheel-contact-transition-smoothing)
7. [Physics Sub-stepping](#7-physics-sub-stepping)
8. [Damping During Force Changes](#8-damping-during-force-changes)
9. [Implementation Priority](#implementation-priority)
10. [Key Parameters to Tune](#key-parameters-to-tune)

---

## 1. Brake Force Interpolation (Most Important)

Your current braking applies force instantly based on `CurrentBrake` input. Add smoothing:

### Code Implementation

```cpp
// In Vehicle.h, add member variables:
float SmoothedBrakeInput = 0.0f;
float BrakeInputSmoothingSpeed = 5.0f; // Adjust for desired response time

// In Tick(), replace instant brake assignment:
SmoothedBrakeInput = FMath::FInterpTo(SmoothedBrakeInput, CurrentBrake, DeltaTime, BrakeInputSmoothingSpeed);
// Use SmoothedBrakeInput instead of CurrentBrake in braking calculations
```

### Why This Helps
- Gradually ramps braking force up/down instead of instant changes
- Prevents sudden weight transfer spikes
- Reduces physics solver shock

---

## 2. Suspension Force Smoothing

Your suspension forces can spike when wheels contact/leave ground. Smooth these:

### Code Implementation

```cpp
// Add to UTire or Vehicle:
float LastSuspensionForce = 0.0f;
float SuspensionForceSmoothingFactor = 0.8f; // 0.0-1.0, higher = more smoothing

// In SuspensionRayCast(), smooth the force:
float RawSuspensionForce = SpringForceMagnitude - DampingForceMagnitude;
float SmoothedSuspensionForce = FMath::Lerp(LastSuspensionForce, RawSuspensionForce, 1.0f - SuspensionForceSmoothingFactor);
LastSuspensionForce = SmoothedSuspensionForce;

// Apply the smoothed force instead of raw
MeshComponent->AddForceAtLocation(SmoothedSuspensionForce * VehicleUpDirection, Tire->getContactPoint());
```

### Why This Helps
- Prevents force spikes when suspension compresses rapidly
- Smooths out ground contact discontinuities
- Reduces vehicle bouncing

---

## 3. Weight Transfer Smoothing

Weight transfer changes abruptly during braking. Smooth the tire load:

### Code Implementation

```cpp
// In UTire, add:
float SmoothedTireLoad = 0.0f;
float TireLoadSmoothingSpeed = 10.0f;

// In Tick(), smooth the load:
SmoothedTireLoad = FMath::FInterpTo(SmoothedTireLoad, TireLoad, DeltaTime, TireLoadSmoothingSpeed);

// Use SmoothedTireLoad for braking calculations instead of raw TireLoad
float MaxBrakingForce = SmoothedTireLoad * Tire->GetFrictionCoefficient();
```

### Why This Helps
- Prevents sudden changes in braking capability
- Reduces pitch instability during weight transfer
- Makes vehicle behavior more predictable

---

## 4. Force Rate Limiting

Limit how quickly forces can change between frames:

### Code Implementation

```cpp
float MaxForceChangeRate = 5000.0f; // Max force change per second

float GetRateLimitedForce(float CurrentForce, float DesiredForce, float DeltaTime)
{
    float MaxChange = MaxForceChangeRate * DeltaTime;
    float ForceDelta = DesiredForce - CurrentForce;
    float ClampedDelta = FMath::Clamp(ForceDelta, -MaxChange, MaxChange);
    return CurrentForce + ClampedDelta;
}

// Usage in braking:
float DesiredBrakingForce = CurrentBrake * MaxBrakingForce;
float RateLimitedBrakingForce = GetRateLimitedForce(LastBrakingForce, DesiredBrakingForce, DeltaTime);
```

### Why This Helps
- Hard limits on force change rates
- Prevents physics explosions from extreme force spikes
- More predictable force application

---

## 5. Exponential Moving Average

Apply IIR (Infinite Impulse Response) filtering to forces:

### Code Implementation

```cpp
float EMASmoothForce(float CurrentForce, float NewForce, float Alpha)
{
    // Alpha: 0.0 = heavy smoothing, 1.0 = no smoothing
    return Alpha * NewForce + (1.0f - Alpha) * CurrentForce;
}

// Usage:
float SmoothedBrakingForce = EMASmoothForce(LastBrakingForce, NewBrakingForce, 0.1f);
```

### Why This Helps
- Simple and efficient smoothing
- Gradually responds to changes
- Filters out high-frequency noise

---

## 6. Wheel Contact Transition Smoothing

When wheels leave/ground contact, forces change abruptly:

### Code Implementation

```cpp
// In UTire, add:
float GroundedBlend = 0.0f; // 0.0 = airborne, 1.0 = fully grounded

// In SuspensionRayCast(), smooth the grounded transition:
if (Compression == 0)
{
    GroundedBlend = FMath::FInterpTo(GroundedBlend, 0.0f, DeltaTime, 10.0f);
    Tire->IsGrounded = false;
}
else
{
    GroundedBlend = FMath::FInterpTo(GroundedBlend, 1.0f, DeltaTime, 10.0f);
    Tire->IsGrounded = true;
}

// Scale forces by GroundedBlend to prevent sudden force changes
float EffectiveBrakingForce = ActualBrakingForce * GroundedBlend;
float EffectiveSuspensionForce = SuspensionForce * GroundedBlend;
```

### Why This Helps
- Prevents sudden force loss/gain when wheels leave/contact ground
- Smooths transitions between airborne and grounded states
- Reduces physics instability on uneven terrain

---

## 7. Physics Sub-stepping

Unreal's physics solver can miss rapid force changes. Enable sub-stepping:

### Code Implementation

```cpp
// In BeginPlay() or constructor:
if (MeshComponent)
{
    MeshComponent->SetPhysicsMaxDepenetrationVelocity(500.0f);
    MeshComponent->SetPhysicsMaxAngularVelocity(360.0f); // degrees per second
    
    // Enable sub-stepping for smoother physics
    MeshComponent->SetBodyPhysicsSimulationEnabled(true);
}
```

### Alternative: Project Settings
You can also enable sub-stepping in Project Settings:
- `Project Settings` → `Physics` → `Sub-stepping`
- Enable `Sub-stepping` and set `Max Delta Time` to appropriate values

### Why This Helps
- Physics solver updates more frequently
- Better captures rapid force changes
- More stable simulation at high speeds

---

## 8. Damping During Force Changes

Add extra damping during rapid force changes:

### Code Implementation

```cpp
// Detect rapid force changes
float ForceChangeRate = FMath::Abs(CurrentForce - LastForce) / DeltaTime;

// Apply additional damping during rapid changes
if (ForceChangeRate > Threshold)
{
    float ExtraDamping = ForceChangeRate * DampingFactor;
    
    // Apply to angular velocity to prevent excessive rotation
    FVector AngularVelocity = MeshComponent->GetPhysicsAngularVelocityInRadians();
    FVector DampingTorque = -AngularVelocity * ExtraDamping;
    MeshComponent->AddTorqueInRadians(DampingTorque);
}
```

### Why This Helps
- Prevents vehicle spinning during force spikes
- Adds stability during rapid force transitions
- Reduces oscillation

---

## Implementation Priority

Implement in this order for maximum impact:

1. **Brake input smoothing** (Section 1) - Most impactful, easiest to implement
2. **Suspension force smoothing** (Section 2) - Prevents ground contact spikes
3. **Tire load smoothing** (Section 3) - Reduces weight transfer jolts
4. **Wheel contact transition smoothing** (Section 6) - Prevents sudden force loss
5. **Force rate limiting** (Section 4) - Adds safety limits
6. **Physics sub-stepping** (Section 7) - System-wide improvement
7. **EMA filtering** (Section 5) - Alternative/complementary approach
8. **Dynamic damping** (Section 8) - Advanced stabilization

---

## Key Parameters to Tune

### Brake Smoothing
- **BrakeInputSmoothingSpeed**: 2.0f-10.0f (lower = smoother but slower response)
  - Start with 5.0f
  - Increase for more responsive braking
  - Decrease for smoother braking

### Suspension Smoothing
- **SuspensionForceSmoothingFactor**: 0.1f-0.9f (higher = more smoothing)
  - Start with 0.7f
  - Increase if suspension feels "jittery"
  - Decrease if suspension feels "mushy"

### Tire Load Smoothing
- **TireLoadSmoothingSpeed**: 5.0f-20.0f (prevents load spikes)
  - Start with 10.0f
  - Increase for faster weight transfer response
  - Decrease for smoother weight transfer

### Contact Transitions
- **GroundedBlend speed**: 5.0f-15.0f (smooths contact transitions)
  - Start with 10.0f
  - Increase for faster ground reaction
  - Decrease for smoother takeoff/landing

### Force Rate Limiting
- **MaxForceChangeRate**: 1000.0f-10000.0f (depends on vehicle mass)
  - Start with 5000.0f
  - Adjust based on vehicle mass and desired behavior
  - Higher for heavier vehicles

---

## Common Issues and Solutions

### Issue: Vehicle still pitches forward when braking
**Solution**: Increase brake input smoothing, add tire load smoothing

### Issue: Vehicle feels "mushy" or unresponsive
**Solution**: Decrease smoothing values, increase interpolation speeds

### Issue: Vehicle bounces on rough terrain
**Solution**: Add suspension force smoothing, enable physics sub-stepping

### Issue: Sudden force spikes when wheels contact ground
**Solution**: Implement wheel contact transition smoothing

### Issue: Vehicle spins out during hard braking
**Solution**: Add force rate limiting, increase damping during force changes

---

## Performance Considerations

- **FInterpTo and FMath::Lerp are very fast** - minimal performance impact
- **Exponential moving average is extremely efficient** - just one multiplication and addition
- **Physics sub-stepping can be expensive** - use judiciously
- **Force rate limiting adds minimal overhead** - simple clamp operations

---

## Testing Recommendations

1. **Test braking at different speeds** - low, medium, high
2. **Test on different surfaces** - flat, uneven, ramps
3. **Test with different vehicle setups** - different masses, CoM heights
4. **Monitor frame rate** - ensure smoothing doesn't impact performance
5. **Use debug visualization** - draw force vectors to see smoothed vs raw forces

---

## Conclusion

The key principle is **gradual force transitions** rather than instant changes. This prevents the physics solver from receiving large, sudden force changes that cause pitching and instability.

Start with the first three techniques (brake smoothing, suspension smoothing, tire load smoothing) as they provide the most benefit with the least complexity. Add additional techniques as needed based on testing results.
