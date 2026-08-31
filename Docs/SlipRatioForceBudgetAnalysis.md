# Front-Wheel Negative Slip Ratio — Diagnosis & Force-Budget Fix

**Date:** 2026-08-28
**Preset affected:** 2009 Chevrolet Corvette GT2 (the only preset in `VehiclePresets.cpp`)
**Files involved:** `Vehicle.cpp` (`UpdateWheel`, `GetTireDriveForce`, `CalculateResistiveForces`), `Tire.cpp` (`UpdateSlipRatio`, `UpdateWheelRotationalVelocity`), `VehiclePresets.cpp`, `CarSettings.cpp`

---

## Part 1 — Why the front wheels report a negative slip ratio

### What negative slip means in this codebase

`UTire::UpdateSlipRatio` (Tire.cpp:89) computes:

```
SlipRatio = (ω·r − v) / max(|ω·r| or |v|, 100 cm/s)
```

Negative means the wheel's surface speed is **slower** than the road passing under it — locked-wheel / braking behavior. The front wheels are the *driven* wheels (`GetTireDriveForce`, Vehicle.cpp:710, returns 0 for rears), so under throttle they should show **positive** slip. Negative front slip means the integrated wheel angular velocity lags the chassis — a structural problem, not a bad coefficient.

### Root cause: the wheel-spin integrator and the chassis see different force budgets

`UpdateWheel` (Vehicle.cpp:629–638) does:

```
NetForce  = GetTireDriveForce(Tire) − Direction × (braking + this tire's rolling resistance + CurrentDrag)
ω̇        = NetForce / (TotalVehicleMass·r + I/r)
```

The denominator is the **single-wheel coupled model** (`ω̇ = F/(M·r + I/r)`), which is only valid when `F` is the *total* force accelerating the *whole* vehicle through *one* wheel. But the code feeds it **one wheel's** traction, while the physics body is pushed by **both** front wheels' traction (applied in Tick at Vehicle.cpp:151), minus four tires' rolling resistance and the drag once.

Comparing accelerations (Chevrolet numbers: M ≈ 1358 kg, I/r² ≈ 0.25 kg — negligible):

- Wheel surface speed: `d(ωr)/dt ≈ (F_t − R − D)/M`
- Chassis: `dv/dt = (2F_t − 4R − D)/M`
- Difference: **(−F_t + 3R)/M** — negative whenever per-wheel traction exceeds ~3× one tire's rolling resistance (~118 N), i.e. any time the car actually accelerates.

Each driven wheel "believes" it alone must spin up the entire 1358 kg car, so with two driven wheels each under-rotates at roughly **half** the chassis' real acceleration. ω·r − v diverges negative from the first frame of throttle. The `Max(0, ω)` clamp in `UpdateWheelRotationalVelocity` (Tire.cpp:167) then pins the wheel at whatever it decays to.

The rears are even worse: with zero drive force their `NetForce` is *always* negative while rolling, so they continuously decelerate toward lock.

### Why the Chevrolet preset makes it glaring

1. **Drag is ~100× too large — and the full-car drag is subtracted inside *every* wheel's NetForce.**
   The preset passes height as `116.3` (VehiclePresets.cpp:29), but `CarSettings.cpp:63` does `Height = NewHeight * 100` expecting meters → a 116.3 m tall car, drag area 244 m² instead of 2.44 m². At ~50 km/h that's ≈ 11.3 kN of drag — roughly equal to the **maximum** per-wheel drive force (~11 kN = 725 N·m × 1.625 × 0.8 × 3.8 / 0.3252 m). Past ~50 km/h, `NetForce` goes negative **at full throttle**: the wheel angularly decelerates while the car keeps rolling, and slip dives toward −1. Even at the correct magnitude, giving each wheel the whole-car drag (4× aggregate) while the chassis receives it once is the same accounting error.

2. **Launch sits in the torque curve's dead zone.**
   `CalculateRPM` = `MinimumStartingRPM + ω·gearing`. With `MinimumStartingRPM = 500`, launch happens at exactly 500 RPM where the curve gives **9 N·m** (it is negative below idle: −34 at 0, −15 at 250). Per-wheel traction ≈ 137 N — barely above the 118 N (3R) threshold, so slip is negative from the start and never recovers.

3. **Gearing works against launch.**
   `GearRatios` are listed ascending {1.625 … 2.923} and `GearIndex` starts at 0 → the car launches in the *tallest* ratio (a real gearbox lists 1st gear as the largest number). Also `SetFromPreset` resets `GearIndex` but never assigns the `GearRatio` member — until the player shifts, it stays at the hardcoded `3.0` from Vehicle.h:73, so the preset ratio isn't even in effect.

### Aggravating details

- **The slip denominator exaggerates it.** In the non-braking branch the ratio is normalized by `|ω·r|` — the *lagging* quantity. A wheel at ωr = 500 cm/s under a chassis doing 900 cm/s reports −0.8, not −0.44. (That's why the braking branch normalizes by vehicle speed instead.)
- **The feedback loop is sign-blind.** `GetTireDriveForce` feeds `Abs(GetSlipRatio())` into the Magic Formula, so growing negative slip never corrects the wheel — it just rides the curve, and past the peak (~|slip| ≈ 0.12 with the current factors) traction decays, worsening the lag. A mild destabilizing loop.

---

## Part 2 — The solution for the force-budget issues

### Governing principle

The chassis and the wheel must share **one** force budget, and the only place they exchange force is the **contact patch**. Currently there are two independent integrators — the physics engine for the chassis, the `UpdateWheel` math for ω — each fed a different ad-hoc list of forces. The plumbing must change so that:

1. The wheel obeys a pure **torque balance around its axle**.
2. The chassis receives only the **tire force Fₓ applied at the contact patch** (plus drag on the body).
3. The tire force is **computed once per frame** from the current slip and is the *same number* in both equations.

### What belongs in each equation

**Wheel (per wheel):**

```
ω̇ = ( τ_drive − Fₓ·r − τ_brake·sgn(ω) − τ_rr ) / I
```

| Term | Belongs? | Why |
|---|---|---|
| Engine torque `τ_drive` (driven wheels only) | ✅ | Torque-curve(RPM) × gear × final drive × efficiency — as **torque**, not "drive force". Don't divide by r and re-merge with vehicle mass. |
| Tire force reaction `Fₓ·r` | ✅ | The term the current model is missing entirely. It makes slip *self-correcting*. |
| Brake torque | ✅ | Already a torque in the preset (`BrakingTorque`). |
| Rolling resistance | as a *small torque* `τ_rr = C_rr·Fz·r` ✅ | Or as a patch force on the chassis — **pick one**. Currently both are done. |
| Aerodynamic drag | ❌ never | Drag acts on the body only. It reaches the wheels *through* the contact patch: drag slows v → slip changes → Fₓ responds. Short-circuiting this by stuffing `CurrentDrag` into each wheel's equation is the 4× overcount. |
| Vehicle mass | ❌ not as denominator | The merged denominator `M·r + I/r` is the whole problem. The wheel's inertia is `I` (≈ 10,960 kg·cm² front, Chevrolet), nothing else. |

**Chassis:** the physics engine only ever sees — Fₓ at each contact patch (already applied), drag via `AddForce` (already), lateral force (already). Drop the separate rolling-resistance chassis forces if `τ_rr` moves to the wheel, or vice versa.

### Why this fixes the sign of the slip

The reaction term `Fₓ·r` closes the feedback loop the current model leaves open:

- **Throttle:** τ_drive spins the wheel up → ωr overshoots v → slip goes **positive** → Fₓ (Magic Formula) pushes the chassis forward *and* the reaction torque pulls the wheel back down → slip settles where `Fₓ·r ≈ τ_drive`. Physically correct driven-wheel behavior.
- **Off-throttle rears (the locked-wheel case):** τ_drive = 0, tiny τ_rr bleeds ω → ωr falls below v → slip slightly negative → Fₓ is negative on the chassis, but its *reaction* torque **speeds the wheel back up**. Free-rolling wheels automatically track road speed. The current model has no restoring term at all, which is why the rears run down to zero.

### Details required for convergence

- **Fₓ must come from slip, not from the engine.** Currently the engine force is clamped by `MaxGrip` and fed as the Magic Formula's *peak*. The peak should be `μ·Fz` (`MaxGrip`), and the **signed** slip ratio is the input. The engine's job is to create slip through torque; the tire curve decides how much force that slip buys.
- **Keep the slip's sign** through the Magic Formula (it is odd-symmetric). Feeding `Abs(GetSlipRatio())` discards direction information — half of why the feedback is blind.
- **Compute Fₓ once per tire per frame** and reuse it for: chassis application, the wheel's reaction torque, friction-circle limiting. Currently `GetTireDriveForce` is evaluated twice per front wheel per tick with slightly different state, so the two integrators consume different forces.
- **Brake zero-crossing:** clamp the wheel's Δω so braking can't push ω *through* zero in one step (classic brake chatter / sign-flip instability). `GetMinimumWheelForce` is a heuristic stab at this; the clean form is limiting Δω, not rescaling a force.
- **Stiffness vs. timestep:** with B ≈ 13 the Magic Formula slope near slip = 0 is steep; explicit Euler at 60 Hz can oscillate around the equilibrium slip. A per-frame clamp on |Δω| (never overshoot rolling speed in one step) is the cheap guard; a relaxation on slip velocity is the proper one.
- **Idle region:** once engine torque acts on the wheel directly, the curve's negative sub-idle values (−34, −15 N·m) will spin the wheel *backward* at standstill. Clamp the curve lookup at idle RPM or add a simple clutch-engagement factor.

### Minimal alternative (keep the merged model)

Numerator = **total** drive force across all driven wheels **minus total** resistances (all four rolling resistences + full drag + brake forces, once); denominator = `M·r + N·I/r` with N = number of driven wheels; both front wheels share the resulting ω. This fixes the sign arithmetic but collapses all wheels into one effective wheel — per-tire slip differences (the reason `Tire` objects exist) become fiction — and it still runs a second chassis integrator beside the physics engine. Use only as a stepping stone to verify the accounting.

### Preset/unit bugs to fix regardless of approach

- Height `116.3` passed where the constructor expects meters (→ 100× drag area).
- `GearRatios` stored ascending → launch in tallest gear.
- `SetFromPreset` resets `GearIndex` but never assigns the `GearRatio` member (stays at hardcoded 3.0 until first shift).

---

## Part 3 — Computing the reaction term Fₓ·r

### It isn't calculated separately — it's the tire force, mirrored

Compute Fₓ once from the Magic Formula at the *signed* slip, then use it in two places:

- **Chassis:** apply `+Fₓ` at the contact patch, along the wheel's forward axis
- **Wheel:** apply torque `τ_reaction = −Fₓ·r`

The minus sign *is* Newton's third law at the contact patch: if the tire pushes the car forward, the patch pushes back on the wheel's rotation by the same force at the lever arm r. One signed number drives both sides, and the sign of Fₓ (inherited from the sign of the slip) makes all four driving states work automatically:

| Situation | Slip κ = (ωr − v)/… | Fₓ on chassis | −Fₓ·r on wheel |
|---|---|---|---|
| Throttle | κ > 0 (wheel over-rotating) | forward (+) | **opposes** engine torque → pulls ω back toward v |
| Coast / free-rolling | κ drifts < 0 | slight backward (−) | **positive** torque → spins the wheel back up to road speed |
| Braking | κ strongly < 0 | backward (−) | positive torque fighting the brake → the lock equilibrium |
| Wheelspin | κ ≫ 0 | forward, past curve peak | large opposing torque ≫ τ_drive → ω is dragged back down |

### The per-frame loop, concretely (using existing pieces)

1. **Update slip first** — from the current ω and v (both from the end of last frame):
   - `v` = hub velocity projected on the wheel's forward vector (the existing `SpeedAtWheel`)
   - `κ = (WheelRotationalVelocity·WheelRadius − v) / max(|WheelRotationalVelocity·WheelRadius|, floor)` — **keep the sign**, drop the `Abs`
2. **Compute Fₓ once:**
   - Peak = `MaxGrip` (`TireGrip × TireLoad` = μ·Fz) — **not** the clamped engine force that `GetTireDriveForce` feeds it today
   - `Fₓ = MagicFormula(MaxGrip, κ, mode)` — `MagicFormula` is odd in x (sin∘atan composition), so signed κ in → signed Fₓ out; no changes needed inside it
   - Mode: pick the factor set from the sign of κ (κ ≥ 0 → `ACCELERATION`, κ < 0 → `BRAKING`)
   - Friction-circle limit Fₓ against the lateral force afterward — the *limited* value is the value used in **both** places. Limiting one side and not the other re-opens the budget hole
3. **Chassis:** `+Fₓ` at `GetContactPoint()` along `Tire->GetForwardVector()`. This **replaces** applying `TireTraction` directly — engine-derived force never touches the chassis anymore; only tire-model force does
4. **Wheel torque balance:**

   ```
   ω̇ = ( τ_drive − Fₓ·r − τ_brake·sgn(ω) − τ_rr ) / I
   ```

   `τ_drive` is torque-curve(RPM) × gear × final drive × efficiency — in torque units (kg·cm²/s²), never divided by r. `I` is the wheel inertia alone (≈ 10,960 kg·cm² front, Chevrolet).
5. Integrate ω; next frame's slip closes the loop.

Ordering change vs. current code: `UpdateWheel` today integrates ω *then* updates slip. Compute slip → force → integrate; this semi-implicit ordering is meaningfully more stable with a steep tire curve.

### Why the magnitudes work out — Chevrolet numbers

Equilibrium condition: `Fₓ·r = τ_drive` — steady slip is whatever κ makes the tire force equal the engine's wheel torque ÷ r:

- **Launch, 500 RPM, 9 N·m:** wheel torque = 9 × 1.625 × 0.8 × 3.8 ≈ 44.5 N·m → required Fₓ = 44.5 / 0.3252 m ≈ **137 N per wheel**. Near κ = 0 the curve's slope is ≈ peak·C·B = 800 kN-units × 2.16 × 13.27, so equilibrium κ ≈ 137/22.9 M ≈ **0.0006 — a tiny positive slip**. Physically correct: a gently driven wheel barely slips.
- **Peak torque, 725 N·m:** wheel torque ≈ 3,592 N·m → Fₓ ≈ 11 kN ≈ MaxGrip — demand sits exactly at the tire's ceiling, so equilibrium lands at/beyond the curve's peak: genuine wheelspin territory. The reaction term discovers this; no grip-clamping of the engine force needed.

Slip stops being a *result* to observe and diagnose — it becomes the *state* the system regulates through the reaction torque.

### Two guards the raw formula needs

- **Δω clamp.** At launch, frame 1: κ = 0 → Fₓ = 0 → one frame of pure engine torque gives Δω ≈ 40 rad/s² × 1/60 ≈ 0.67 rad/s → κ jumps past 0.2 → Fₓ ≈ 3.5 kN → reaction torque ≫ τ_drive → ω gets yanked back, possibly undershooting → chatter. Clamp the frame's Δω so ω·r can't overshoot the current v by more than slip should move per frame (equivalently: limit toward `ω_target = v/r`). The same clamp, mirrored, prevents brake-induced zero-crossing chatter — it replaces what `GetMinimumWheelForce` was papering over.
- **Low-speed denominator.** The 100 cm/s floor means ωr = 20 cm/s reads as κ = 0.2 — near-peak force from almost nothing. Fine for launching (it's why a car creeps) but it amplifies chatter; the Δω clamp keeps it tame. The floor itself is standard practice; keep it.

**Consistency rule throughout:** r and the Fₓ used must be identical in the chassis application and the wheel reaction — same frame, same evaluation, same rolling radius. The moment the two sides see different numbers, you're back to two divergent budgets, which is the disease this whole document diagnoses.

---

## Part 4 — Implementation (code)

Fitted to the existing classes and units (cm, force = kg·cm/s², torque = kg·cm²/s²).

### 4.1 `UTire` — compute Fₓ once, from the signed slip

```cpp
// Tire.h
    //Calculates the signed longitudinal tire force from the current slip ratio
    float CalculateLongitudinalForce(const float LateralForceMagnitude) const;
```

```cpp
// Tire.cpp
float UTire::CalculateLongitudinalForce(const float LateralForceMagnitude) const
{
    if (!IsGrounded)
    {
        return 0.0f;
    }

    //The slip's sign selects the drive/brake factor set. MagicFormula is odd in x,
    //so a signed slip in gives a signed force out - no Abs() anywhere
    const int Mode = SlipRatio >= 0.0f ? WHEELMODE::ACCELERATION : WHEELMODE::BRAKING;
    const float RawForce = MagicFormula(MaxGrip, SlipRatio, Mode);

    //Limit against the lateral force ONCE. This limited value is what BOTH the
    //chassis application and the wheel reaction torque must use - limiting one
    //side only re-opens the force-budget hole
    return FrictionCircle(RawForce, LateralForceMagnitude, false);
}
```

Notes:

- The peak is `MaxGrip` (μ·Fz) — **not** a clamped engine force.
- `UpdateSlipRatio` already stores a *signed* slip, so it needs no change; only the `Abs(GetSlipRatio())` call sites disappear.

### 4.2 `UTire` — the reaction term and the wheel torque balance

Replaces the merged-mass `UpdateWheelRotationalVelocity(AngularAcceleration, …)`:

```cpp
// Tire.h
    //Updates the wheel's spin from a pure torque balance around the axle
    void UpdateWheelRotationalVelocity(const float DriveTorque, const float TireForce, const float DeltaTime);
```

```cpp
// Tire.cpp
void UTire::UpdateWheelRotationalVelocity(const float DriveTorque, const float TireForce, const float DeltaTime)
{
    if (DeltaTime <= 0.0f)
    {
        return;
    }
    //Free rotation while airborne
    if (!IsGrounded)
    {
        WheelRotationalVelocity *= FMath::Pow(WheelDamper, DeltaTime);
        return;
    }

    const float Radius = SuspensionSettings.WheelRadius;
    if (Radius <= 0.0f || Inertia <= 0.0f)
    {
        return;
    }

    //--- THE REACTION TERM: the tire force this frame, mirrored onto the wheel ---
    const float ReactionTorque = -TireForce * Radius;

    //Brake and rolling resistance oppose whichever way the wheel is turning
    const float RotationSign = WheelRotationalVelocity >= 0.0f ? 1.0f : -1.0f;
    const float ResistiveTorque = -RotationSign *
        (BrakingTorque + GetRollingResistance() * Radius);

    //Pure torque balance: engine/brake torques act on the wheel, the tire force
    //reacts on it. No vehicle mass, no drag - those live on the chassis only
    const float NetTorque = DriveTorque + ReactionTorque + ResistiveTorque;
    float DeltaOmega = NetTorque / Inertia * DeltaTime;

    //--- Δω clamp: the curve is stiff (B ≈ 13), so cap how far ω·r may move per frame.
    //5% of the current surface-speed scale: ~5 cm/s at standstill (kills launch chatter),
    //~27 cm/s at 20 km/h (well above real acceleration, far below the slip yank)
    const float MaxSurfaceSpeedChange = 0.05f *
        FMath::Max(FMath::Abs(WheelRotationalVelocity * Radius), 100.0f);
    DeltaOmega = FMath::Clamp(DeltaOmega,
        -MaxSurfaceSpeedChange / Radius, MaxSurfaceSpeedChange / Radius);

    //Braking/rolling must not shove the wheel backwards through zero in one step
    if (WheelRotationalVelocity > 0.0f)
    {
        DeltaOmega = FMath::Max(DeltaOmega, -WheelRotationalVelocity);
    }
    else if (WheelRotationalVelocity < 0.0f)
    {
        DeltaOmega = FMath::Min(DeltaOmega, -WheelRotationalVelocity);
    }

    WheelRotationalVelocity += DeltaOmega;
}
```

Passing `TireForce` in explicitly (rather than caching it on the tire) makes it structurally impossible for the two sides to see different numbers.

### 4.3 `AVehicle::UpdateWheel` — the wiring, in the right order

```cpp
// Vehicle.cpp
void AVehicle::UpdateWheel(UTire* Tire, float DeltaTime)
{
    const FVector SocketLocation = SkeletalMesh->GetSocketLocation(Tire->SocketName);
    const FVector WheelForward = Tire->GetForwardVector();
    const FVector VelocityAtWheel = PhysicMesh->GetPhysicsLinearVelocityAtPoint(SocketLocation);
    const float SpeedAtWheel = FVector::DotProduct(VelocityAtWheel, WheelForward);

    Tire->UpdateSteering(CurrentSteeringAngle);

    //1) Slip FIRST, from the current state (signed - the Abs() call sites are gone)
    Tire->UpdateSlipRatio(SpeedAtWheel, IsBraking);

    //2) ONE tire force per wheel per frame
    const float LateralMagnitude = Tire->GetLateralForceVector().Size();
    const float TireForce = Tire->CalculateLongitudinalForce(LateralMagnitude);

    //3) Chassis: the SAME force at the contact patch. This replaces applying
    //   TireTraction / braking / rolling-resistance forces to the body directly
    ApplyLocationForce(TireForce * WheelForward, Tire->GetContactPoint(), true);

    //4) Wheel: engine torque in (torque units - never divided by radius),
    //   reaction term built from the same TireForce
    const float DriveTorque = Tire->IsFrontTire
        ? GetWheelTorque(Tire) * GearRatio * CurrentPresets.FinalDriveRatio
              * CurrentPresets.DrivetrainEfficiency * CurrentThrottle
        : 0.0f; //rears are undriven: the reaction term alone keeps them rolling

    Tire->UpdateWheelRotationalVelocity(DriveTorque, TireForce, DeltaTime);
}
```

### 4.4 What this deletes from the current code

- The `Denominator = TotalVehicleMass * r + I/r` block and the `NetForce = GetTireDriveForce − Direction×(brake + rolling + CurrentDrag)` line — drag and full-vehicle mass leave the wheel equation entirely.
- The front-tire traction loop in `Tick` (Vehicle.cpp:140–156) — traction is now applied per-wheel in step 3.
- `ApplyBraking`'s chassis force — braking now reaches the chassis as negative Fₓ from negative slip, produced by the brake torque in the wheel equation.
- `GetTireDriveForce` / `GetDriveForce` as chassis-force sources (`GetWheelTorque` survives as the τ_drive source).

### 4.5 Sanity check against the worked example (Part 3)

At 20 km/h, 2000 RPM (Chevrolet numbers): slip comes in at κ ≈ +0.0325 → `TireForce ≈ 617,900` → `ReactionTorque = -617,900 × 32.52 ≈ -20,094,000` against `DriveTorque ≈ +20,007,000`. That near-cancellation with a small negative residual trimming ω is exactly the equilibrium from the hand calculation — if those two magnitudes track each other on screen, the wiring is right.
