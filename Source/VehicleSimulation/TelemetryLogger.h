// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Tire.h"
#include "CoreMinimal.h"

/**
 * 
 */
class VEHICLESIMULATION_API TelemetryLogger
{
public:
    TelemetryLogger();
	~TelemetryLogger();

    // The function that will be called by the timer
    void LogDataToCSV(float Timestamp, float Speed, float Acceleration, float Throttle, float Brake,
        float Steer, float Drag, float Pitch, float Heave, 
        TArray<UTire*> Tires, float DeltaTime, TFunction <float(UTire* Tire)> RollingResistanceFunction);

    // Helper to write a single line to the file
    void AppendToFile(const FString& Line);

    //Changes the current logging path
    void ChangeLoggerFile(const FName VehiclePresetName);
private:
    // Name of the file that currently stores the logs
    FString CurrentLoggingFile = TEXT("Default.csv");

    // Full path to the file
    FString FullFilePath = FPaths::ProjectSavedDir() / TEXT("Telemetry") ;

    // Creates the CSV file for the current logger with a header if it doesn't exist
    void InitialiseLoggerCSV();
};
