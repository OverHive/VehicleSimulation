# Front Lift on Throttle - Issue Diagnosis

**Date:** 2026-07-27  
**Issue:** Forward throttle causes the front of the vehicle to lift and lose traction  
**Status:** 🔴 Identified - Not Yet Fixed

---

## Executive Summary

When throttle is applied, the front of the vehicle lifts up, causing a loss of front tire traction. This is caused by three fundamental implementation issues:

1. **Incorrect drive wheel configuration** (Front-Wheel Drive instead of Rear-Wheel Drive)
2. **Missing anti-squat mechanics during acceleration**
3. **Positive feedback loop in pitch-based load reduction**

---

## Root Cause Analysis

### Issue 1: Wrong Drive Wheels (CRITICAL)

**Location:** `Source/VehicleSimulation/Tire.cpp:154-157`

```cpp
float UTire::GetTraction(const float ThrottleForce) const
{
    return IsFrontTire && IsGrounded ? FMath::Clamp(ThrottleForce, -MaximumWheelTraction, MaximumWheelTraction) : 0;
}
```

**Problem:** Throttle force is only applied to **front tires** (Front-Wheel Drive configuration). When forward force is applied at the front contact patch (ground level, below the center of gravity), it creates a pitch moment that rotates the vehicle nose-up.

**Why this causes lift:**
- Forward force at ground level → moment arm = CG height
- This creates a nose-up rotation torque
- Front suspension extends as the vehicle pitches up
- Front tire load decreases → less traction → more wheelspin

---

### Issue 2: Anti-Dive Applied Incorrectly During Acceleration

**Location:** `Source/VehicleSimulation/Vehicle.cpp:175-184, 438`

```cpp
if (CurrentBrake > 0.0f)
{
    IsBraking = true;
    AntiDiveFactor = (1.0f - AntiDivePercentage);  // Reduces weight transfer
}
else
{
    IsBraking = false;
    AntiDiveFactor = 1.0f;  // FULL weight transfer during acceleration
}
```

And in weight transfer calculation (line 438):
```cpp
LongitudinalWeightTransfer *= AntiDiveFactor;
```

**Problem:** 
- During **braking**: Anti-dive correctly reduces weight transfer (front dives less)
- During **acceleration**: No anti-squat mechanism, so FULL weight transfer occurs to the rear

**Current behavior:**
| Condition | AntiDiveFactor | Effect on Front Load |
|-----------|----------------|---------------------|
| Braking | 0.7 (70%) | Reduced transfer, front maintains load |
| Acceleration | 1.0 (100%) | Full transfer to rear, front loses load |

**What's missing:** The `AntiSquatPercentage` variable (defined at 25% in Vehicle.h) is never used!

---

### Issue 3: Pitch-Based Load Reduction Creates Feedback Loop

**Location:** `Source/VehicleSimulation/Vehicle.cpp:447-450`

```cpp
// Add pitch-induced weight redistribution
float PitchWeightTransfer = (VehicleMass * Gravity * PitchAngle * CentreOfGravityHeight) / WheelBaseLength;
DynamicFrontLoad -= PitchWeightTransfer;
DynamicRearLoad += PitchWeightTransfer;
```

**Problem:** This creates a **positive feedback loop**:

1. Front starts to lift (pitch angle increases)
2. Pitch angle reduces front load calculation
3. Less front load → less traction → less forward force
4. Vehicle continues to pitch up from the drive moment
5. Loop repeats, front eventually loses all traction

**Why this is problematic:**
- The pitch weight transfer is **unidirectional** (only subtracts from front)
- No mechanism prevents it from reducing front load to zero
- Real vehicles have suspension geometry that limits this effect

---

## Physics Breakdown

### Expected vs Actual Behavior

#### What SHOULD Happen (RWD Vehicle)
| Phase | Weight Distribution | Suspension Behavior |
|-------|-------------------|---------------------|
| Static | 50/50 (static distribution) | Both at ride height |
| Acceleration | ~45/55 (slight rear bias) | Front extends slightly, rear compresses |
| Braking | ~60/40 (front bias) | Front compresses, rear extends |

#### What ACTUALLY Happens (Current Implementation)
| Phase | Weight Distribution | Suspension Behavior |
|-------|-------------------|---------------------|
| Static | 50/50 | Both at ride height |
| Acceleration | ~20/80 (extreme rear bias) | Front extends significantly, lifts off ground |
| Braking | ~55/45 (dive reduced) | Front compresses (anti-dive working) |

### Torque Diagram

```
During Acceleration (Current FWD):

     ↑ (Pitch moment from front drive force)
    ___CG (30cm height)
   |   |
   |___|
    ↑↑← Front Drive Force (at ground level)
    ║║
   ═════

Result: Nose rotates UP (front lifts)


Expected RWD:

    ___CG
   |   |
   |___|
   →→↑ Rear Drive Force (at ground level)
     ║║
    ═════

Result: Nose rotates DOWN slightly (front stays planted)
```

---

## Recommended Fixes

### Fix 1: Change to Rear-Wheel Drive ⭐ (Recommended)

**File:** `Tire.cpp:154-157`

**Current:**
```cpp
float UTire::GetTraction(const float ThrottleForce) const
{
    return IsFrontTire && IsGrounded ? ... : 0;
}
```

**Fix to:**
```cpp
float UTire::GetTraction(const float ThrottleForce) const
{
    return !IsFrontTire && IsGrounded ? FMath::Clamp(ThrottleForce, -MaximumWheelTraction, MaximumWheelTraction) : 0;
}
```

**Why:** Most vehicles are RWD for this exact reason - rear drive keeps the front planted during acceleration.

---

### Fix 2: Implement Anti-Squat During Acceleration

**File:** `Vehicle.cpp:175-184`

**Current:**
```cpp
if (CurrentBrake > 0.0f)
{
    IsBraking = true;
    AntiDiveFactor = (1.0f - AntiDivePercentage);
}
else
{
    IsBraking = false;
    AntiDiveFactor = 1.0f;  // Full transfer
}
```

**Fix to:**
```cpp
if (CurrentBrake > 0.0f)
{
    IsBraking = true;
    AntiDiveFactor = (1.0f - AntiDivePercentage);
}
else if (CurrentThrottle > 0.0f)
{
    IsBraking = false;
    AntiDiveFactor = (1.0f - AntiSquatPercentage);  // Use anti-squat!
}
else
{
    IsBraking = false;
    AntiDiveFactor = 1.0f;
}
```

**Note:** `AntiSquatPercentage` is already defined as 25% in `Vehicle.h:151`

---

### Fix 3: Clamp Pitch-Based Load Reduction

**File:** `Vehicle.cpp:447-450`

**Add clamping to prevent unrealistic load reduction:**

```cpp
float PitchWeightTransfer = (VehicleMass * Gravity * PitchAngle * CentreOfGravityHeight) / WheelBaseLength;

// NEW: Clamp pitch effect to prevent complete front unload
PitchWeightTransfer = FMath::Clamp(PitchWeightTransfer, -StaticFrontLoad * 0.5f, StaticFrontLoad * 0.5f);

DynamicFrontLoad -= PitchWeightTransfer;
DynamicRearLoad += PitchWeightTransfer;
```

**Why:** This ensures pitch effects can't reduce front load by more than 50%, maintaining minimum traction.

---

### Alternative: If You WANT Front-Wheel Drive

If FWD is intentional, you need additional compensating mechanisms:

1. **Increase front suspension stiffness significantly:**
   - Set `FrontSuspensionStiffness` to ~2-3x rear stiffness
   - This resists the pitch-up moment

2. **Lower the center of gravity:**
   - Reduce `CentreOfGravityHeight` from 30cm to ~20-25cm
   - Reduces the moment arm causing rotation

3. **Add a drive torque compensator:**
   - Calculate the pitch moment from drive force
   - Apply counter-torque or increase front spring force proportionally

---

## Testing Checklist

After implementing fixes, verify:

- [ ] Throttle at full speed: Front stays on ground
- [ ] Hard acceleration: Minimal front lift, rear compresses
- [ ] Braking: Front compresses with controlled dive
- [ ] Combined acceleration + steering: Front tires maintain grip
- [ ] Hill climb: Front doesn't unload excessively
- [ ] Debug output shows front tire load > 0 during throttle

---

## Related Code Locations

| File | Lines | Description |
|------|-------|-------------|
| `Tire.cpp` | 154-157 | Drive force application (CRITICAL) |
| `Vehicle.cpp` | 175-184 | Anti-dive factor setting |
| `Vehicle.cpp` | 426-464 | Weight transfer calculation |
| `Vehicle.cpp` | 447-450 | Pitch-based load adjustment |
| `Vehicle.h` | 148-154 | Anti-dive/anti-squat parameters |
| `Vehicle.h` | 76-80 | CG height and wheelbase dimensions |

---

## Notes

- The anti-dive mechanism works correctly for braking
- The `AntiSquatPercentage` variable exists but is unused
- Consider making drive wheel configuration editable in Blueprint
- Monitor front tire load in debug output (message 4-5)

---

**Generated:** 2026-07-27  
**Project:** VehicleSimulation  
**Analysis by:** Claude Code
