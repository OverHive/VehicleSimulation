// Fill out your copyright notice in the Description page of Project Settings.


#include "TelemetryLogger.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
TelemetryLogger::TelemetryLogger()
{
}

TelemetryLogger::~TelemetryLogger()
{
}

void TelemetryLogger::LogDataToCSV(float Timestamp, float Speed, float Throttle,float Brake, 
    float Steer, float Drag, float Pitch, float Heave,
    TArray<UTire*> Tires , float DeltaTime, TFunction <float(UTire* Tire)> TractionFunction, TFunction <float(UTire* Tire)> RollingResistanceFunction)
{


    //First get the vehicle body state
    FString DataLine = FString::Printf(TEXT("%5.1f, %5.1f,%5.1f, %5.1f, %5.1f, %5.1f,%5.1f, %5.1f"), Timestamp, Speed, Throttle, Brake, Steer, Drag, Pitch, Heave);
    //Then get the state of each wheel
    for(UTire* &Tire:Tires)
    {
        TEXT("FR_Fz, FR_slipRatio, FR_SlipAngle, FR_Fx, FR_Fy, SuspensionForce, FR_RollingResistance, FR_ContactX, FR_ContactY ,FR_ContactZ,");
        float Fz = Tire->GetTireLoad();
        float SlipRatio = Tire->GetSlipRatio();
        float SlipAngle = Tire->GetSlipAngle();
        float Fx = TractionFunction(Tire);
        float Fy = Tire->GetLateralForceVector(DeltaTime).Size();
        float SuspensionForce = Tire->GetSuspensionForce();
        float RollingResistance = RollingResistanceFunction(Tire);
        FVector Contact = Tire->GetContactPoint();
        DataLine += FString::Printf(TEXT(",%5.1f ,%5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f"), Fz, SlipRatio, SlipAngle, Fx, Fy, SuspensionForce, RollingResistance, Contact.X, Contact.Y, Contact.Z);
    }
    // Write the line to the file
    AppendToFile(DataLine);
}

void TelemetryLogger::AppendToFile(const FString& Line)
{
    //Load the content of the file
    FString ExistingTelemetry;
    if (!FFileHelper::LoadFileToString(ExistingTelemetry, *FullFilePath))
    {
        return;
    }
    // Ensure proper newline handling
    if (!ExistingTelemetry.IsEmpty() && !ExistingTelemetry.EndsWith(TEXT("\n")))
    {
        ExistingTelemetry.Append(TEXT("\n"));
    }

    // Append the new line
    ExistingTelemetry.Append(Line);

    // Ensure the file ends with a newline
    if (!ExistingTelemetry.EndsWith(TEXT("\n")))
    {
        ExistingTelemetry.Append(TEXT("\n"));
    }
    // Save the content
    FFileHelper::SaveStringToFile(ExistingTelemetry, *FullFilePath);
}

void TelemetryLogger::ChangeLoggerFile(const FName VehiclePresetName)
{
	CurrentLoggingFile = VehiclePresetName.ToString() + ".csv";
	FullFilePath = FPaths::ProjectSavedDir() / TEXT("Telemetry")/CurrentLoggingFile;
    //Ensure that there is a file to write to
    InitialiseLoggerCSV();
}

void TelemetryLogger::InitialiseLoggerCSV()
{
    // Create the file csv if  does not exist
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FullFilePath))
    {
        // Create header:
        FString Header = TEXT("Timestamp, Speed, Throttle, Brake, Steer, Drag, Pitch, Heave,");
            Header += TEXT("FR_Fz, FR_slipRatio, FR_SlipAngle, FR_Fx, FR_Fy, SuspensionForce, FR_RollingResistance, FR_ContactX, FR_ContactY ,FR_ContactZ,");
            Header += TEXT("FL_Fz, FL_slipRatio, FL_SlipAngle, FL_Fx, FL_Fy, SuspensionForce, FL_RollingResistance, FL_ContactX, FL_ContactY ,FL_ContactZ,");
            Header += TEXT("RR_Fz, RR_slipRatio, RR_SlipAngle, RR_Fx, RR_Fy, SuspensionForce, RR_RollingResistance, RR_ContactX, RR_ContactY ,RR_ContactZ,");
            Header += TEXT("RL_Fz, RL_slipRatio, RL_SlipAngle, RL_Fx, RL_Fy, SuspensionForce, RL_RollingResistance, RL_ContactX, RL_ContactY ,RL_ContactZ\n");
        FFileHelper::SaveStringToFile(Header, *FullFilePath);
    }
}
