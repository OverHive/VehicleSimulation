# Anti-Dive Implementation Guide for Half-Car Model

## Overview

Anti-dive is a suspension geometry concept that reduces the front-end "dive" (nose-down pitch) during braking. In real vehicles, this is achieved through suspension linkage angles that cause some braking forces to create counter-acting moments.

## Understanding Anti-Dive

### What is Anti-Dive?

Anti-dive is a suspension design feature that:
- Reduces front-end dive during braking
- Improves vehicle stability and control
- Maintains better suspension geometry during hard braking
- Works through suspension linkage geometry rather than springs

### How It Works in Real Vehicles

In actual vehicle suspension systems:
1. Control arms are angled upward toward the front
2. During braking, the braking force creates an upward moment on the front suspension
3. This counteracts the natural pitch moment from weight transfer
4. Result: Reduced nose-dive during braking

## Implementation Strategy for Half-Car Model

### 1. Add Anti-Dive Parameters to Vehicle.h

Add these parameters to your vehicle class:

```cpp
// Anti-dive and anti-squat settings
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
float AntiDivePercentage = 0.3f;        // 0.0-1.0 (30% default for street vehicles)

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")  
float AntiSquatPercentage = 0.25f;      // 0.0-1.0 (25% default)

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
float AntiDiveAngle = 20.0f;           // degrees (typical 15-25°)
```

**Typical Values:**
- Street vehicles: 20-50% anti-dive
- Performance vehicles: 30-60% anti-dive
- Racing vehicles: 50-100% anti-dive (but compromises ride comfort)

### 2. Modify Weight Transfer Calculation

**Location:** `CalculatePitchWeightTransfer()` function (lines 406-443)

**Current Calculation:**
```cpp
float LongitudinalWeightTransfer = (VehicleMass * LongitudinalAcceleration * CentreOfGravityHeight) / WheelBaseLength;
```

**Enhanced with Anti-Dive:**
- Detect braking condition (negative longitudinal acceleration)
- Apply anti-dive factor only during braking
- Reduce the effective weight transfer to front axle

**Implementation Concept:**
```cpp
// Detect braking condition
bool IsBraking = (LongitudinalAcceleration < 0.0f) && (CurrentBrake > 0.0f);

// Apply anti-dive reduction during braking
float AntiDiveFactor = IsBraking ? (1.0f - AntiDivePercentage) : 1.0f;
float AdjustedWeightTransfer = LongitudinalWeightTransfer * AntiDiveFactor;
```

### 3. Update Pitch Dynamics Calculation

**Location:** `CalculatePitchAndHeaveDynamics()` function (lines 445-501)

**Current Pitch Moment:**
```cpp
float WeightTransferMoment = VehicleMass * LongitudinalAcceleration * CentreOfGravityHeight;
PitchMoment += WeightTransferMoment;
```

**Enhanced with Anti-Dive:**
- Apply anti-dive compensation to pitch moment
- Use different factors for braking vs acceleration
- Consider the sign of longitudinal acceleration

**Implementation Concept:**
```cpp
// Determine if braking or accelerating
bool IsBraking = LongitudinalAcceleration < 0.0f;

// Apply appropriate anti-dive/anti-squat factor
float AntiFactor = IsBraking ? (1.0f - AntiDivePercentage) : (1.0f - AntiSquatPercentage);

// Calculate compensated moment
float CompensatedMoment = VehicleMass * LongitudinalAcceleration * CentreOfGravityHeight * AntiFactor;
PitchMoment += CompensatedMoment;
```

### 4. Suspension Force Compensation

**Location:** Modify suspension force calculations in `ApplySuspensionForceEffects()`

**Concept:**
Anti-dive geometry in real suspension creates additional forces:
- During braking: Front suspension receives upward force component
- This force counteracts the spring compression that causes dive

**Implementation Approach:**
```cpp
// Apply anti-dive force compensation during braking
if (CurrentBrake > 0.0f && LongitudinalAcceleration < 0.0f)
{
    // Calculate anti-dive force component
    float AntiDiveForceMagnitude = CurrentBrake * BrakeForce * AntiDivePercentage * FMath::Tan(FMath::DegreesToRadians(AntiDiveAngle));
    
    // Apply to front suspension points
    for (UTire*& Tire : AllTires)
    {
        if (Tire->IsFrontTire && Tire->IsGrounded)
        {
            FVector AntiDiveForce = UpVector * AntiDiveForceMagnitude;
            ApplyLocationForce(AntiDiveForce, SkeletalMesh->GetSocketLocation(Tire->SocketName));
        }
    }
}
```

### 5. Key Formulas and Calculations

#### Anti-Dive Force Formula
```
AntiDiveForce = BrakingForce × AntiDivePercentage × tan(AntiDiveAngle)
```

#### Weight Transfer with Anti-Dive
```
EffectiveWeightTransfer = WeightTransfer × (1.0 - AntiDivePercentage)  [during braking]
```

#### Pitch Moment Compensation
```
CompensatedPitchMoment = PitchMoment × (1.0 - AntiDiveFactor)
```

## Implementation Locations in Vehicle.cpp

### Primary Modification Points:

1. **Line 406-443**: `CalculatePitchWeightTransfer()`
   - Add anti-dive factor to longitudinal weight transfer
   - Detect braking conditions
   - Apply compensation to front/rear dynamic loads

2. **Line 445-501**: `CalculatePitchAndHeaveDynamics()`
   - Modify weight transfer moment calculation
   - Apply anti-dive/anti-squat compensation
   - Adjust pitch acceleration calculation

3. **Line 172-189**: Braking force application section
   - Add anti-dive force compensation
   - Apply upward force to front suspension during braking

### Suggested Code Flow:

```cpp
void AVehicle::CalculatePitchWeightTransfer(const float LongitudinalAcceleration, const float LateralAcceleration)
{
    // ... existing static load calculations ...
    
    // Detect braking condition
    bool IsBraking = (LongitudinalAcceleration < 0.0f) && (CurrentBrake > 0.0f);
    
    // Apply anti-dive reduction during braking
    float AntiDiveFactor = IsBraking ? (1.0f - AntiDivePercentage) : 1.0f;
    
    // Dynamic weight transfer with anti-dive compensation
    float LongitudinalWeightTransfer = (VehicleMass * LongitudinalAcceleration * CentreOfGravityHeight) / WheelBaseLength;
    float CompensatedWeightTransfer = LongitudinalWeightTransfer * AntiDiveFactor;
    
    // ... rest of the function with compensated values ...
}
```

## Testing and Tuning

### Testing Procedure:

1. **Start with conservative values**: 20-30% anti-dive
2. **Test braking behavior**: Apply brakes at various speeds
3. **Observe pitch response**: Vehicle should dive less but not pitch up
4. **Fine-tune percentage**: Adjust based on desired handling characteristics
5. **Test different scenarios**: 
   - Light braking
   - Hard braking
   - Braking while turning
   - Braking on different surfaces

### Tuning Guidelines:

**Too Much Anti-Dive (>70%):**
- Vehicle may pitch UP under hard braking
- Harsh ride quality
- Reduced suspension compliance

**Too Little Anti-Dive (<10%):**
- Excessive front-end dive
- Poor braking stability
- Uneven tire loading during braking

**Optimal Range:**
- Street cars: 20-40%
- Sports cars: 30-50%
- Race cars: 40-60%

### Performance Considerations:

- Anti-dice affects ride comfort
- Higher anti-dive = stiffer feeling during braking
- Balance between stability and comfort
- Consider vehicle weight and center of gravity height

## Integration with Existing Systems

### Compatibility with Current Features:

1. **Suspension Raycast**: Anti-dive works alongside existing suspension calculations
2. **Weight Transfer**: Enhances existing weight transfer logic
3. **Pitch Dynamics**: Integrates with current pitch calculation
4. **Braking System**: Complements existing brake force application

### Potential Conflicts:

- Ensure anti-dive doesn't interfere with natural suspension movement
- Balance with existing pitch damping
- Don't override safety limits in pitch clamping

## Advanced Features (Optional)

### Dynamic Anti-Dive:
- Vary anti-dive percentage based on braking intensity
- More anti-dive for hard braking, less for light braking

### Speed-Dependent Anti-Dive:
- Reduce anti-dive at low speeds for better comfort
- Increase anti-dive at high speeds for stability

### Surface Adaptation:
- Adjust anti-dive based on road surface friction
- Less anti-dive on low-friction surfaces

## Summary

Anti-dive implementation modifies the weight transfer and pitch calculations to reduce braking-induced pitch moments, simulating the effect of suspension linkage geometry that opposes dive during braking operations.

### Key Implementation Steps:
1. Add anti-dive parameters to Vehicle.h
2. Modify weight transfer calculation in `CalculatePitchWeightTransfer()`
3. Update pitch dynamics in `CalculatePitchAndHeaveDynamics()`
4. Add anti-dive force compensation during braking
5. Test and tune for desired handling characteristics

### Benefits:
- Improved braking stability
- Better vehicle control under hard braking
- More consistent tire loading during braking
- Enhanced driver confidence

### Trade-offs:
- Reduced ride comfort during braking
- More complex tuning requirements
- Potential for harshness if over-applied

## Next Steps

1. Implement the parameter additions to Vehicle.h
2. Modify the calculation functions in Vehicle.cpp
3. Test with various anti-dive percentages
4. Fine-tune based on vehicle behavior
5. Document final settings for your specific vehicle setup