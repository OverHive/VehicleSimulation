# Pitch/Heave Dynamics Fix

## Problem Diagnosis

The vehicle behaves as if it's driving over bumps on smooth ground. There are actually **two separate issues** here:

---

### Issue #1: What's Actually Causing the Bumping

The bumping is likely caused by several factors in the **working suspension system**:

**Potential Root Causes:**

1. **Spring-Damper Oscillation**
   - Wrong spring stiffness or damping coefficient values
   - Under-damped system creates oscillation
   - Check `FrontSuspensionStiffness`, `RearSuspensionStiffness`, `FrontSuspensionDamping`, `RearSuspensionDamping`

2. **Raycast Inconsistency** 
   - Raycasts may hit slightly different ground points each frame
   - Small variations in hit location cause force variations
   - Force variations cause body movement
   - Body movement changes next frame's raycasts
   - **Creates feedback loop**

3. **Force Application Timing**
   - Suspension forces applied after physics timestep
   - Unreal physics already moved the body
   - Custom forces push it back
   - **Creates fighting between systems**

4. **Frame Lag in Weight Transfer**
   - `PitchAngle` used for weight transfer is from previous frame
   - Weight distribution doesn't match current body state
   - May contribute to instability

5. **Numerical Instability**
   - Integration method may introduce errors
   - Euler integration can oscillate with stiff springs
   - Unreal uses more sophisticated physics integration

**The Conflict:**

The bumping comes from **two systems trying to control the same vehicle body**:

- **Custom suspension:** Applies forces based on raycasts
- **Unreal physics:** Responds to forces and handles collision

When these fight or get out of sync, oscillation occurs.

---

### Issue #2: Pitch/Heave Values ARE Being Used (But Inconsistently)

The calculated values **ARE actually used** - I was wrong to say they're unused:

**`PitchAngle` usage (line 443):**
```cpp
float PitchWeightTransfer = (VehicleMass * Gravity * PitchAngle * CentreOfGravityHeight) / WheelBaseLength;
DynamicFrontLoad -= PitchWeightTransfer;
DynamicRearLoad += PitchWeightTransfer;
```

**`HeavePosition` usage:** Nowhere - this truly is unused

**Execution Order Problem:**
1. `CalculatePitchWeightTransfer()` - **USES** `PitchAngle` (from previous frame)
2. `SuspensionRayCast()` - updates tire compression
3. `CalculatePitchAndHeaveDynamics()` - **UPDATES** `PitchAngle` (for next frame)
4. `ApplySuspensionForceEffects()` - applies forces

This creates a **one-frame lag**: weight transfer uses pitch angle from the previous frame.

---

### Summary

- **Issue #1 (CAUSES bumping):** Raycast suspension system + Unreal physics engine oscillating against each other
- **Issue #2 (INCONSISTENT usage):** `PitchAngle` is used for weight transfer (with frame lag), `HeavePosition` is completely unused
- **Issue #3 (STRUCTURAL):** Attempting to run custom body dynamics alongside Unreal's physics engine

---

## Solution Options

### Option 1: Apply Pitch/Heave to Vehicle Transform

**What it does:** 
- **Fixes Issue #1:** Attempts to make custom dynamics control the vehicle
- **Fixes Issue #2:** Makes the pitch/heave calculations actually do something
- **Result:** Manual transform control + Unreal physics (likely to conflict)

**Implementation approach:**
```cpp
// After line 512 in CalculatePitchAndHeaveDynamics()

// Apply pitch to vehicle rotation
FRotator NewRotation = GetActorRotation();
NewRotation.Pitch = FMath::RadiansToDegrees(PitchAngle);
SetActorRotation(NewRotation);

// Apply heave to vehicle position
FVector NewLocation = GetActorLocation();
NewLocation.Z += HeavePosition;
SetActorLocation(NewLocation);
```

**Why this might fail:**
- You're using `SetSimulatePhysics(true)` (line 29) - this means Unreal controls the transform
- Manually setting position/rotation will fight with physics engine
- Can cause jitter, instability, or physics engine rejection
- Unreal may override your transforms, making your calculations pointless
- Creates WORSE oscillation than current bumping

---

### Option 2: Use Pitch/Heave to Offset Raycasts

**What it does:** 
- **Fixes Issue #1:** Makes raycasts consistent with calculated dynamics (reduces oscillation)
- **Fixes Issue #2:** Makes the pitch/heave calculations actually affect the vehicle
- **Result:** Hybrid system where custom calculations guide raycasts

**Implementation approach:**
```cpp
// In SuspensionRayCast(), replace line 244:
FVector StartLocation = SkeletalMesh->GetSocketLocation(Tire->SocketName);

// Apply heave offset
StartLocation.Z += HeavePosition;

// Apply pitch rotation offset
FVector OffsetFromCenter = StartLocation - GetActorLocation();
OffsetFromCenter = OffsetFromCenter.RotateAngleAxis(
    FMath::RadiansToDegrees(PitchAngle),
    GetActorRightVector()
);
StartLocation = GetActorLocation() + OffsetFromCenter;
```

**Why this might work:**
- Raycasts originate from where suspension model "thinks" vehicle is
- Creates consistency between calculations and raycasting
- Suspension forces match calculated dynamics
- Reduces feedback loop (raycasts don't jump around as much)
- Doesn't fight with Unreal's physics engine directly

**Potential issues:**
- Still running two physics models in parallel (custom calculations + Unreal physics)
- May need to dampen pitch/heave calculations to prevent feedback loops
- More complex than Option 3
- Still has potential for oscillation if values aren't tuned perfectly

---

### Option 3: Remove Pitch/Heave Dynamics ❌ **NOT VIABLE**

**What it does:**
- Attempts to let Unreal's physics engine handle all body dynamics
- **BUT THIS BREAKS THE WEIGHT TRANSFER SYSTEM**

**Why this doesn't work:**
- `PitchAngle` is **actively used** in `CalculatePitchWeightTransfer()` (line 443)
- Removing `CalculatePitchAndHeaveDynamics()` would leave `PitchAngle` stuck at 0
- This would break pitch-based weight transfer calculations
- Front/rear weight distribution would be incorrect during acceleration/braking

**When this could work:**
- ONLY if you also remove the pitch-based weight transfer (line 443)
- But then you'd lose realistic weight transfer effects
- Not recommended unless you're redesigning the entire physics system

---

### Option 4: Hybrid Approach - Pitch/Heave for Load Distribution Only

**What it does:**
- **Partially fixes Issue #1:** Keeps custom calculations but clarifies their purpose
- **Partially fixes Issue #2:** Makes the unused calculations serve a specific purpose
- **Result:** Unreal handles body physics, custom code handles tire load distribution

**Rationale:**
The pitch/heave values are used in weight transfer calculations (line 443):
```cpp
float PitchWeightTransfer = (VehicleMass * Gravity * PitchAngle * CentreOfGravityHeight) / WheelBaseLength;
```

You can keep these calculations for tire load distribution without trying to physically move the vehicle.

**Implementation approach:**
- Keep current pitch/heave calculations
- Clarify that these are for **load distribution only**, not body control
- Let Unreal handle actual body dynamics
- Use pitch/heave values only for calculating weight distribution to tires

**Changes needed:**
- Don't apply pitch/heave to transform
- Don't offset raycasts with pitch/heave  
- Keep weight transfer calculations as-is
- This is essentially the current code but with clarified intent/comments

**Why this might work:**
- Separates concerns between systems
- Unreal handles physics body (position/rotation)
- Custom code handles tire load distribution (based on simulated body state)
- Minimal changes to existing code
- May still cause bumping if Unreal's actual body state differs from calculated state

**Key insight:**
This option accepts that the pitch/heave calculations are **simulated values for tire load purposes**, not actual body state. The vehicle's real body state is whatever Unreal's physics engine calculates.

---

## Recommendation Summary

### First Choice: **Option 2** (Offset raycasts with pitch/heave)

**Best because:**
- Makes the calculated pitch/heave values actually useful
- Reduces raycast oscillation (raycasts follow calculated body state)
- Maintains existing weight transfer system
- Creates consistency between calculations and raycasting

**Expected outcome:**
- Reduced bumping (raycasts stay consistent with calculated dynamics)
- Pitch/heave calculations serve a real purpose
- Maintains current weight transfer functionality
- May require fine-tuning to prevent new oscillation

### Second Choice: **Option 4** (Keep current, accept limitations)

**Use if:**
- The bumping isn't severe enough to warrant changes
- You want to avoid introducing new bugs
- Time constraints prevent major refactoring

**Expected outcome:**
- Current behavior maintained (including bumping)
- Add code comments to clarify the frame lag in weight transfer
- Consider the bumping an acceptable limitation

### Third Choice: **Option 1** (Apply to transform) ❌

**Use only if:**
- You're willing to disable `SetSimulatePhysics(true)` 
- You want to write a completely custom physics engine
- You understand this is a major rewrite

**Expected outcome:**
- Likely WORSE instability than current bumping
- Requires writing custom collision, gravity, and momentum systems
- Extremely complex and time-consuming

### Not Viable: **Option 3** (Remove pitch/heave)

**Why not:**
- Would break the weight transfer system that depends on `PitchAngle`
- Only works if you also remove pitch-based weight transfer
- Loses realistic weight transfer effects

---

## Implementation Priority

1. **Start with Option 2** - Makes pitch/heave calculations useful, reduces oscillation
2. **Test thoroughly** - Verify smooth driving, consistent behavior, no new oscillation
3. **If unsatisfied, try Option 4** - Accept current limitations with better documentation
4. **Never use Option 1** - Unless you're writing a completely custom physics engine
5. **Don't use Option 3** - Will break your weight transfer system

## Quick Decision Guide

- **Best balance of fix vs. risk?** → Option 2 (offset raycasts)
- **Accept current behavior?** → Option 4 (add documentation, live with bumping)
- **Major rewrite acceptable?** → Option 1 (but need to disable Unreal physics first)
- **❌ Won't work?** → Option 3 (breaks weight transfer system)

---

## Testing Checklist

After implementing any solution, verify:

- [ ] Vehicle drives smoothly on flat ground
- [ ] No oscillation or bumping when stationary
- [ ] Pitch responds realistically to acceleration/braking
- [ ] Suspension compresses appropriately over bumps
- [ ] Weight transfer affects steering (front-heavy vs rear-heavy)
- [ ] No jitter or instability at high speeds
- [ ] Raycasts still hit ground correctly

---

## Additional Notes

### Current Anti-Dive Implementation

Lines 100-101 show anti-dive mechanics:
```cpp
ActualBrakingForce *= AntiDivePercentage* FMath::Tan(FMath::DegreesToRadians(AntiDiveAngle));
```

This might interact with pitch dynamics - worth testing if anti-dive feels correct after changes.

### Heave Position Limits

Line 519 clamps heave position:
```cpp
HeavePosition = FMath::Clamp(HeavePosition, -20.0f, 20.0f);
```

If keeping Option 3, this limit can be removed since Unreal handles heave naturally.

### Damping Values

Lines 499 and 516 use hardcoded damping:
```cpp
PitchVelocity *= PitchDamping;  // Variable defined in header
HeaveVelocity *= 0.98f;         // Hardcoded!
```

If keeping custom dynamics, consider making HeaveDamping configurable like PitchDamping.

---

## Related Files

- `Vehicle.cpp` - Main vehicle physics implementation
- `Vehicle.h` - Vehicle class definition
- `Tire.cpp` - Individual tire physics
- `Tire.h` - Tire class definition
