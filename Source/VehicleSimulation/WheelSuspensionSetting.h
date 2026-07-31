// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheelSuspensionSetting.generated.h"

/**
 * Per-wheel suspension configuration.
 */
USTRUCT(BlueprintType)
struct VEHICLESIMULATION_API FWheelSuspensionSetting
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SuspensionLength = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpringStiffness = 14300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DampingCoefficient = 1400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WheelRadius = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TireVerticalStiffness = 20000000.0f;


};
