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
    FVector AttachmentOffset = FVector::ZeroVector; // Local offset from vehicle centre

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SuspensionLength = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpringStiffness = 50000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DampingCoefficient = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WheelRadius = 20.0f;


};
