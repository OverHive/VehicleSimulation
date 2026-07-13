# Wheel-Ground Interaction Implementation Guide

## Overview

This document describes how to restructure the vehicle simulation system so that wheels interact with the ground as separate physics bodies instead of using raycast-based ground detection on a single vehicle body.

## Current vs. Desired Architecture

### Current Structure
- **Single Physics Body**: `MeshComponent` handles all physics simulation
- **Non-Physical Wheels**: `UTire` components are visual components without physics
- **Raycast Ground Detection**: `SuspensionRayCast()` performs line traces to detect ground
- **Force Application**: Forces applied to main body at calculated contact points via `AddForceAtLocation()`

### Desired Structure
- **Multiple Physics Bodies**: Main chassis + separate physics bodies for each wheel
- **Physical Wheels**: Each wheel has its own collision and physics simulation
- **Direct Collision**: Wheels naturally collide with ground geometry
- **Constraint-Based Suspension**: Physics constraints connect wheels to chassis

## Implementation Steps

### 1. Modify UTire Class for Physics

Add physics capabilities to the `UTire` class in `Tire.h`:

```cpp
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VEHICLESIMULATION_API UTire : public UStaticMeshComponent
{
    GENERATED_BODY()
    
public:
    // ... existing members ...
    
    // Add physics simulation initialization
    virtual void BeginPlay() override;
    
    // Physics constraint for suspension (connects wheel to chassis)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics")
    class UPhysicsConstraintComponent* SuspensionConstraint;
    
    // Ground detection using physics collision
    bool IsGrounded() const;
    
    // Set reference to main chassis component
    void SetChassisReference(UPrimitiveComponent* Chassis) { ChassisComponent = Chassis; }
    
private:
    UPrimitiveComponent* ChassisComponent;
    float WheelMass = 15.0f; // Typical car wheel mass in kg
};
```

### 2. Enable Physics Simulation on Wheels

Implement physics initialization in `Tire.cpp`:

```cpp
void UTire::BeginPlay()
{
    Super::BeginPlay();
    
    // Enable physics simulation for the wheel
    SetSimulatePhysics(true);
    SetEnableGravity(true);
    SetMassOverrideInKg(NAME_None, WheelMass, true);
    
    // Configure collision for ground interaction
    SetCollisionObjectType(ECC_Pawn);
    SetCollisionResponseToAllChannels(ECR_Block);
    SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    
    // Create physics constraint for suspension
    SuspensionConstraint = NewObject<UPhysicsConstraintComponent>(this, TEXT("SuspensionConstraint"));
    if (SuspensionConstraint)
    {
        SuspensionConstraint->RegisterComponent();
        
        // Attach to both components
        SuspensionConstraint->SetConstrainedComponents(
            ChassisComponent, NAME_None, 
            this, NAME_None
        );
        
        // Configure linear spring (suspension)
        SuspensionConstraint->SetLinearDriveParams(
            5000.0f,  // Stiffness
            100.0f,   // Damping
            0.0f      // Force limit
        );
        
        SuspensionConstraint->SetLinearPositionDrive(true, true, false);
        SuspensionConstraint->SetLinearMotionType(LCM_Free);
        
        // Configure angular constraints
        SuspensionConstraint->SetAngularSwing1Limit(EACM_Locked, 0.0f);
        SuspensionConstraint->SetAngularSwing2Limit(EACM_Locked, 0.0f);
        SuspensionConstraint->SetAngularTwistLimit(EACM_Locked, 0.0f);
    }
}

bool UTire::IsGrounded() const
{
    // Check if wheel is in contact with ground
    FVector WheelLocation = GetComponentLocation();
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredComponent(this);
    Params.AddIgnoredActor(GetOwner());
    
    return GetWorld()->LineTraceSingleByChannel(
        Hit, 
        WheelLocation, 
        WheelLocation - FVector(0, 0, (TireRadius * 2.0f)), 
        ECC_WorldStatic, 
        Params
    );
}
```

### 3. Update Vehicle Construction

Modify vehicle constructor in `Vehicle.cpp`:

```cpp
AVehicle::AVehicle()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Create main chassis component
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;
    
    // Configure chassis physics
    MeshComponent->SetSimulatePhysics(true);
    MeshComponent->SetEnableGravity(true);
    MeshComponent->SetMassOverrideInKg(NAME_None, VehicleMass, true);
    MeshComponent->SetAngularDamping(2.5f);
    
    SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleComponent"));
    SkeletalMesh->SetupAttachment(MeshComponent);
    
    // Create wheels as separate physics components
    FrontRightTire = CreateDefaultSubobject<UTire>(TEXT("Front-Right Tire"));
    FrontRightTire->IsFrontTire = true;
    
    FrontLeftTire = CreateDefaultSubobject<UTire>(TEXT("Front-Left Tire"));
    FrontLeftTire->IsFrontTire = true;
    
    RearRightTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Right Tire"));
    RearRightTire->IsRightTire = true;
    
    RearLeftTire = CreateDefaultSubobject<UTire>(TEXT("Rear-Left Tire"));
    RearLeftTire->IsRightTire = true;
    
    CreateTires();
}
```

### 4. Update Wheel Creation and Attachment

```cpp
void AVehicle::CreateTires()
{
    AllTires.Add(FrontRightTire);
    AllTires.Add(FrontLeftTire);
    AllTires.Add(RearRightTire);
    AllTires.Add(RearLeftTire);
    
    for (int i = 0; i < AllTires.Num(); i++)
    {
        if (AllTires[i] != nullptr && SkeletalMesh)
        {
            // Initial attachment for setup
            AllTires[i]->SetupAttachment(SkeletalMesh, FName(socketNames[i]));
            
            // Configure wheel physics
            AllTires[i]->SetSimulatePhysics(true);
            AllTires[i]->SetEnableGravity(true);
            AllTires[i]->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            
            // Set chassis reference
            AllTires[i]->SetChassisReference(MeshComponent);
            
            // Store socket name
            AllTires[i]->SocketName = socketNames[i];
            
            // Configure tire parameters
            AllTires[i]->UpdateFrictionCoefficient(1.0);
            AllTires[i]->UpdateVehicleParameters(
                VehicleMass, WheelBaseLength, TrackWidth,
                DistanceOfCentreOfGravityToFrontAxis, 
                DistanceOfCentreOfGravityToRearAxis, 
                CentreOfGravityHeight
            );
        }
    }
}
```

### 5. Simplify Tick Function for Physics-Based Wheels

Replace raycast suspension with direct wheel physics:

```cpp
void AVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Update steering
    CurrentSteeringAngle = FMath::FInterpTo(
        CurrentSteeringAngle, 
        CurrentSteering * MaxSteeringAngle, 
        DeltaTime, 
        SteeringInterpSpeed
    );
    
    // Apply steering damping when released
    if (FMath::Abs(CurrentSteering) < SteeringReleaseThreshold)
    {
        FVector AngularVelocity = MeshComponent->GetPhysicsAngularVelocityInRadians();
        float YawAngularVelocity = AngularVelocity.Z;
        
        FVector LinearVelocity = MeshComponent->GetPhysicsLinearVelocity();
        FVector RightVector = GetActorRightVector();
        float LateralVelocity = FVector::DotProduct(LinearVelocity, RightVector);
        
        float SpeedFactor = FMath::Clamp(FMath::Abs(LinearVelocity.Size()) / 1000.0f, 0.1f, 1.0f);
        float DampingCoefficient = AngularDampingWhenSteeringReleased * VehicleMass * SpeedFactor * 10.0f;
        
        FVector CounterTorque = FVector(0.0f, 0.0f, -YawAngularVelocity * DampingCoefficient);
        MeshComponent->AddTorqueInRadians(CounterTorque);
        
        float LateralCorrectionForce = -LateralVelocity * DampingCoefficient * 0.5f;
        MeshComponent->AddForce(RightVector * LateralCorrectionForce);
    }
    
    CurrentVelocity = MeshComponent->GetPhysicsLinearVelocity();
    CalculateResistiveForce(CurrentVelocity);
    
    // Apply braking
    if (CurrentBrake > 0.0f)
    {
        FVector BrakingForce = -MeshComponent->GetPhysicsLinearVelocity().GetSafeNormal() * 
                              (CurrentBrake * BrakeForce);
        MeshComponent->AddForce(BrakingForce, NAME_None, false);
    }
    
    // Update wheel physics
    for (UTire* Tire : AllTires)
    {
        if (Tire && Tire->IsGrounded())
        {
            // Update steering for front wheels
            Tire->UpdateSteering(CurrentSteeringAngle);
            
            // Apply traction forces to wheel
            FVector WheelForward = Tire->GetForwardVector();
            float TractionForce = CurrentThrottle * ThrottleForce / 4.0f;
            Tire->AddForce(WheelForward * TractionForce);
            
            // Update tire load calculations
            FVector Acceleration = (CurrentVelocity - LastVelocity) / DeltaTime;
            float LongitudinalAcceleration = FVector::DotProduct(Acceleration, GetActorForwardVector());
            float LateralAcceleration = FVector::DotProduct(Acceleration, GetActorRightVector());
            Tire->UpdateTireLoad(LongitudinalAcceleration, LateralAcceleration);
        }
    }
    
    // Update debugging
    LastVelocity = CurrentVelocity;
    UpdateDebugDisplay();
}
```

### 6. Remove Raycast Suspension

The old `SuspensionRayCast()` function can be removed or significantly simplified since physics constraints handle suspension naturally.

## Benefits of Physics-Based Wheel Interaction

1. **More Realistic Physics**: Wheels naturally interact with ground geometry and obstacles
2. **Better Suspension Behavior**: Physics constraints provide realistic spring-damper dynamics
3. **Simplified Code**: Eliminates complex raycasting and manual force calculations
4. **Improved Terrain Handling**: Wheels naturally conform to uneven ground surfaces
5. **Better Collision Response**: Wheels can roll over obstacles naturally

## Implementation Considerations

### Performance
- **More Physics Bodies**: 4 additional physics bodies increase computational load
- **Constraint Solving**: Physics constraints require additional solver iterations
- **Collision Detection**: More collision pairs to process

### Stability
- **Tuning Required**: Spring stiffness, damping, and constraint limits need careful adjustment
- **Mass Distribution**: Proper mass distribution between chassis and wheels is critical
- **Constraint Stability**: Multiple constraints can cause instability if not properly configured

### Configuration Tips
1. **Start Simple**: Test with one wheel first, then add others
2. **Conservative Stiffness**: Begin with lower spring stiffness and increase gradually
3. **Mass Balance**: Ensure wheel mass is appropriate relative to chassis mass (typically 2-3% of total vehicle mass)
4. **Constraint Limits**: Use angular constraints to prevent excessive wheel rotation

## Testing Strategy

1. **Single Wheel Test**: Implement one wheel first and verify ground interaction
2. **Suspension Tuning**: Adjust spring and damping parameters for desired behavior
3. **Full Vehicle Test**: Add all four wheels and test vehicle stability
4. **Performance Test**: Monitor frame rate and physics simulation time
5. **Terrain Test**: Test on various ground surfaces and slopes

## Migration Path

1. **Phase 1**: Implement physics on one wheel while keeping raycast suspension on others
2. **Phase 2**: Migrate all wheels to physics-based interaction
3. **Phase 3**: Remove raycast suspension code
4. **Phase 4**: Fine-tune physics parameters and constraints

## Troubleshooting

### Issues and Solutions

**Wheels fall through ground**: 
- Ensure wheel collision is properly enabled
- Check collision channels and response settings
- Verify wheel radius is set correctly

**Vehicle explodes/unstable**:
- Reduce spring stiffness
- Increase damping
- Check mass distribution
- Verify constraint setup

**Poor traction**:
- Adjust friction coefficients
- Increase wheel mass for better ground contact
- Tune suspension geometry

**Performance issues**:
- Reduce physics sub-stepping
- Simplify collision geometry
- Optimize constraint solver iterations

## Conclusion

Migrating to physics-based wheel interaction provides more realistic and natural vehicle behavior. While the implementation requires careful tuning, the resulting simulation quality improvement is significant for most vehicle simulation applications.
