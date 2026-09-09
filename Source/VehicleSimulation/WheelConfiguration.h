// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheelConfiguration.generated.h"
static TArray<FName>  SocketNames{ "Socket_FR","Socket_FL","Socket_RR","Socket_RL" };
/**
 *The configuration data for a wheel
 */
USTRUCT(BlueprintType)
struct VEHICLESIMULATION_API FWheelConfiguration
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel")
	FVector Position = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel")
	bool IsDriveWheel = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel")
	bool IsRightWheel = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel")
	bool IsSteerWheel = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel")
	bool IsRearWheel = false;
	FWheelConfiguration() {};
	FWheelConfiguration(const FVector NewPosition, const bool Drivable, const bool OnRight, const bool CanSteer, const bool OnRearAxis)
	{
		Position = NewPosition;
		IsDriveWheel = Drivable;
		IsRightWheel = OnRight;
		IsSteerWheel = CanSteer;
		IsRearWheel = OnRearAxis;
	}
};
