# Physical Tire Force Architecture — Implementation Plan

> Design discussion from 2026-09-01. Line numbers refer to the current state of
> `Vehicle.cpp` / `Tire.cpp` and will drift as the code changes.

## Problem

The current model splits the contact-patch force into two inconsistent curves:

- **Chassis** is pushed by `LongitudinalForce` = drive − brake, where drive is
  `MagicFormula(ClampToTireGrip(torque), slipRatio)` (Vehicle.cpp:139, applied at
  Vehicle.cpp:147) — a *demand-shaped, slip-scaled* force.
- **Wheel** is back-driven by `TireForce` = `MagicFormula(MaxGrip, slipRatio)`
  (Vehicle.cpp:609) — the *full-grip* reaction. It only appears in
  `UpdateWheelRotationalVelocity`, never on the body.

No double-counting (good), but the two curves have different peaks
(`min(demand, MaxGrip)` vs `MaxGrip`), so the wheel's torque balance settles in
the wrong place.

### Symptom

Wheel equilibrium is where `NetForce = 0` (Tire.cpp:225):

```
F_app(κ) − F_react(κ) − RR = 0
min(T, μN)·s(κ) − μN·s(κ) = RR
−(μN − T)·s(κ) = RR        →  s(κ) < 0
```

Under throttle (with `T < μN`), equilibrium slip is **negative** — the driven
wheels sit on the braking side of zero slip. The margin `(μN − T)` shrinks as
throttle approaches grip, so near the limit the front (drive) axle settles at
noticeably negative slip: `GetTireReactionForce` runs in `BRAKING` mode and the
on-screen `slipRatio` reads negative while accelerating.

Wheelspin and lockup also cannot emerge naturally — the chassis force is a
shaped clamp rather than the physical consequence of slip.

## Target equations

```
Chassis:  F_applied = F_x                          (circled reaction, blended at low speed)
Wheel:    I·dω/dt  = (T_drive − sign(ω)·T_brake)/R − F_x − sign(ω)·RR
F_x       = FrictionCircle(MagicFormula(MaxGrip, κ), |F_y|)
```

The engine is not connected to the ground: torque goes into the wheel, slip
develops at the contact patch, and the resulting friction force is **one force
with two jobs** — it pushes the car *and* back-drives the wheel. Same `F_x` in
both equations, opposite roles.

Key insight for implementation: `UpdateWheelRotationalVelocity`
(Tire.cpp:211-246) already implements the physical wheel equation —
`NetForce = Drive − TireForce − RR` is exactly right. The work is rewiring what
gets passed in and where the chassis force comes from.

## Step 0 (prerequisite): fix the rate-limit scale

The limiter at Tire.cpp:232-236 caps surface-speed change at 5% of *wheel
surface speed* per second, with a `100 cm/s` floor. Problems:

- Binds hardest when the wheel is far from equilibrium (lockup recovery with
  the car above `FormulaThreshold` is capped at `0.05 m/s²` — effectively frozen
  until the car drops below the threshold and the kinematic path resets `ω`).
- Acts as an accidental, uncontrolled ABS near lock.
- The floor is surface-speed units → radius-dependent in angular terms, so
  front/rear axles get different limits from the same constant.

Fix: scale by state that doesn't vanish when the wheel slows. Pass ground speed
in (available as `LongitudinalVelocity` in `UpdateWheel`, Vehicle.cpp:597):

```cpp
const float SurfaceSpeed = WheelRotationalVelocity * Radius;
// Ground speed keeps the scale alive when the wheel itself is slow/locked;
// the epsilon only guards against a fully stationary vehicle.
const float SpeedScale = FMath::Max3(FMath::Abs(SurfaceSpeed), FMath::Abs(GroundSpeed), 10.0f);
const float MaxSurfaceSpeedChange = WheelResponseRate * DeltaTime * SpeedScale; // tunable, ~0.05–0.2
```

The floor drops from "dominant behavioral constant" to "non-zero epsilon".

Principled upgrade (optional, later): clamp by distance to equilibrium instead —

```cpp
const float SlipSpeed = SurfaceSpeed - GroundSpeed;
const float MaxSurfaceSpeedChange = FMath::Abs(GroundSpeedDelta)
    + SlipConvergenceRate * DeltaTime * FMath::Abs(SlipSpeed);
```

- `GroundSpeedDelta` (change in ground speed since last frame, one stored member
  on `UTire`) lets the wheel keep pace with braking/acceleration transients.
- `SlipConvergenceRate` closes the slip gap with a bounded time constant — pick
  it to mimic a real relaxation length (~0.1–0.3 s) for physical transients.
- Cannot overshoot the equilibrium by construction; needs no magic floor.

In the current architecture the limiter only grooms slip readouts; in the
physical one it governs how fast `F_x` can respond to torque, so this fix is a
prerequisite — at 5%/s every input would feel laggy.

## Step 1: Demote drive/brake demands to wheel-side only

```cpp
// GetTireDriveForce loses its grip clamp AND its MagicFormula shaping:
float AVehicle::GetTireDriveForce(UTire* Tire)
{
    if (!Tire->IsFrontTire) return 0;
    return GetDriveForce(GetWheelTorque(Tire)) * CurrentThrottle;   // pure torque/radius demand
}
```

The slip dynamics now *are* the grip limiter — clamping the demand would
suppress wheelspin, which is the whole point. Same for brake:
`Tire->GetBrakingForce() * CurrentBrake` (torque/radius, already exists at
Tire.cpp:39) replaces `GetUnSignedTireBrakingForce` on the chassis.

Numerically the unclamped demand is safe: the anti-flip clamp (Tire.cpp:238)
and the Step-0 limiter bound `Δω`.

Dead code after this step:

- `GetUnSignedTireBrakingForce` (Vehicle.cpp:705-714)
- `GetMinimumWheelForce` (Vehicle.cpp:721-744)
- `IsLongitudinalControlled` selection logic (Vehicle.cpp:105-115) — the
  friction circle on the (F_x, F_y) pair replaces the mode switching.

## Step 2: Restructure the per-tire loop in Tick

Order must become: **slip → force → chassis application → wheel integration**.
`F_x` must come from fresh slip, and the wheel must consume the same `F_x` the
chassis got. (Currently slip is updated inside `UpdateWheel` at
Vehicle.cpp:604-605 *after* the chassis force is applied — one frame of latency
that matters once the loop is closed through the chassis.)

```cpp
// 1. Geometry & velocities (as now)
float LongitudinalVelocity = ...;                                   // ground speed at wheel
float LateralVelocity = ...;

// 2. Slip from last frame's ω
Tire->UpdateSteering(CurrentSteeringAngle);
Tire->UpdateSlipAngle(LongitudinalVelocity, LateralVelocity);
Tire->UpdateSlipRatio(LongitudinalVelocity, IsBraking);

// 3. The one true tire force (circled pair)
float Fx = Tire->GetTireReactionForce(FyRaw, false);                // MF(MaxGrip, κ), circled vs Fy
FVector LateralFriction = Tire->CircleLateralAgainst(Fx);           // Fy clamped to sqrt(MaxGrip² − Fx²)

// 4. Chassis gets the reaction, blended with demand at low speed
float Demand = GetTireDriveForce(Tire)
    - FMath::Sign(LongitudinalVelocity) * Tire->GetBrakingForce() * CurrentBrake;
ApplyWheelForce(Tire, FMath::Lerp(Demand, Fx, MagicBlend), WheelForward, false);
ApplyLocationForce(LateralFriction, Tire->GetContactPoint(), true);

// 5. Wheel integration consumes the same Fx
if (IsUsingMagicFormula)
    Tire->UpdateWheelRotationalVelocity(DemandAtWheel, Fx,
        GetTireRollingResistance(Tire, DeltaTime), LongitudinalVelocity, DeltaTime);
else
    Tire->ClampToVehicleWheelSpeed(LongitudinalVelocity, DeltaTime);
```

Notes:

- `DemandAtWheel` uses `sign(ω)` for the brake term, not `sign(SpeedAtWheel)`
  — brake torque opposes *wheel* rotation, which is what makes lockup emerge
  correctly.
- `CircleLateralAgainst` is the one new Tire method. Today lateral is circled
  against the *uncircled* longitudinal demand (Vehicle.cpp:153-162); circling
  both against each other makes the contact-patch force a consistent vector
  inside `MaxGrip`.

## Step 3: The threshold blend

Below `FormulaThreshold` the kinematic wheel gives `κ = 0` → `F_x = 0` → no
propulsion or braking. Blend demand force out / reaction force in across a
speed band:

```cpp
float AVehicle::GetMagicBlend() const  // 0 = demand-driven (arcade), 1 = reaction-driven (physical)
{
    float SpeedKmh = FMath::Abs(FVector::DotProduct(CurrentVelocity, PhysicMesh->GetForwardVector() * 0.036f));
    return FMath::SmoothStep(FormulaThreshold * 0.5f, FormulaThreshold, SpeedKmh);
}
```

Below threshold: exactly today's behavior (demand force, kinematic wheel).
Above: the physical loop takes over.

Optional refinement — warm-start on upward crossing: set
`ω = v/R × (1 + κ_demand)` with `κ_demand ≈ Demand / MaxGrip` so `F_x` starts
near the demand instead of ramping from zero. Without it there is a brief force
dip for the ~0.1–0.3 s it takes slip to build.

## Step 4: What deliberately does not move

- **Rolling resistance stays wheel-side** (Tire.cpp:225). It now correctly
  reaches the chassis through the slip loop: coasting at steady state gives
  `F_x = −RR`, so the car gently decelerates. One home, no double-count.
- **`ApplyLocationForce`'s counter-torque trick** (Vehicle.cpp:663-673) is
  untouched — `F_x` rides the same channel the demand force did, so pitch
  behavior doesn't change.
- **`CalculateRPM` gets more correct for free**: it reads `ω`
  (Vehicle.cpp:755), so during wheelspin the RPM climbs, the torque curve
  droops, and demand falls — the engine becomes its own wheelspin governor.

## Verification checklist

1. **Steady throttle below grip**: `slipRatio` now small *positive* (it is
   negative today — the peak-mismatch symptom).
2. **Full throttle on low-μ** (drop surface friction via
   `UpdateFrictionCoefficient`): `ω` runs away past the curve peak, `F_x`
   drops, car accelerates less than grip-limited — a real burnout.
3. **Brake to lock**: `κ` goes large negative, wheel reaches `ω ≈ 0` and stays
   there (small chatter expected — brake flips sign with `ω`, bounded by the
   anti-flip clamp), car slides at the curve's sliding asymptote.
4. **Coast**: car decelerates by drag + a small `RR`-sized `F_x`, no creep.
5. **Threshold crossing** both directions under throttle and brake: no force
   jumps.
6. **Watch `κ` oscillating around 0** under light throttle —
   `GetTireReactionForce` switches factor sets at `κ = 0` (Tire.cpp:61), and if
   the ACCELERATION/BRAKING factors differ much there will be chatter now that
   the loop is closed through the chassis.

## Order of work

1. Rate-limit scale fix (Step 0) — small, independently testable.
2. The rewiring (Steps 1-2) — biggest diff, mostly deletions in Vehicle.cpp.
3. Blend + warm-start (Step 3).
4. Later, if transients feel off: slip relaxation length (low-pass the slip
   ratio toward its kinematic target), which would let the blend band shrink
   and addresses loop stiffness at the source.

Expected diff shape: Tire.cpp gains one small method (`CircleLateralAgainst`)
and a parameter (ground speed); Vehicle.cpp loses `GetUnSignedTireBrakingForce`,
`GetMinimumWheelForce`, the `IsLongitudinalControlled` machinery, and the
MF-shaping in `GetTireDriveForce`.
