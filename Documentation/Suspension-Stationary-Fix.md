# Suspension Force Fix for Stationary Vehicles

## Problem
When the vehicle is not moving, suspension forces continue to be applied, which can cause unnecessary physics calculations and potential instability.

## Solution
Modify the `SuspensionRayCast()` function in `Vehicle.cpp` to skip suspension force calculations and application when the vehicle is stationary.

## Implementation

### Location
`Vehicle.cpp` - `SuspensionRayCast()` function (around line 228)

### Code Change

Insert the following code block **after** the compression clamping and grounded check (after line 225), **before** the spring force calculation (before line 228):

```cpp
// Check if vehicle is stationary (very low or no movement)
float VehicleSpeed = CurrentVelocity.Size();
bool IsVehicleStationary = VehicleSpeed < 0.1f; // Threshold for "stationary"

// If stationary, skip suspension forces to prevent unnecessary physics jitter
if (IsVehicleStationary)
{
    // Optionally still update wheel grounded state but don't apply forces
    if (Compression > 0.0f)
    {
        Tire->IsGrounded = true;
        Tire->UpdateWheelSuspension(FVector::ZeroVector, Hit.Location);
    }
    else
    {
        Tire->IsGrounded = false;
    }
    continue; // Skip applying suspension forces
}
```

## How It Works

1. **Velocity Check**: Uses `CurrentVelocity.Size()` to get the total vehicle speed
2. **Stationary Threshold**: 0.1 cm/s threshold determines if the vehicle is "stationary"
3. **Skip Forces**: Uses `continue` to bypass spring/damping force calculations when stationary
4. **Maintain State**: Still updates tire grounded state so other systems work correctly

## Benefits

- Prevents unnecessary physics calculations when vehicle is parked
- Reduces potential physics instability at low speeds
- Maintains proper tire state for other vehicle systems
- Simple and efficient implementation

## Parameters

You can adjust the stationary threshold based on your needs:
- `0.1f` - Current threshold (0.1 cm/s)
- Increase for more aggressive cutoff
- Decrease for more precise low-speed handling

## Notes

- `CurrentVelocity` is already calculated in the `Tick()` function (line 110)
- This change affects all tires in the suspension system
- The modification preserves the existing suspension behavior when the vehicle is moving
