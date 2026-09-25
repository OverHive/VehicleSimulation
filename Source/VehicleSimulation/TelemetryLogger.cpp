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

void TelemetryLogger::LogDataToCSV(float Timestamp, float Speed, float AccelerationX, float AccelerationY, float Throttle, float Brake,
	float Steer, float Drag, float Pitch, float Heave,
	TArray<UTire*> Tires, float DeltaTime, TFunction <float(UTire* Tire)> RollingResistanceFunction)
{


	//First get the vehicle body state
	FString DataLine = FString::Printf(TEXT("%5.1f, %5.1f,%5.1f,%5.1f, %5.1f, %5.1f, %5.1f,%5.1f, %5.1f, %5.1f"), Timestamp, Speed/100, AccelerationX/100, AccelerationY / 100, Throttle, Brake, Steer, Drag/100, Pitch, Heave/100);
	//Then get the state of each wheel
	for (UTire*& Tire : Tires)
	{
		float Fz = Tire->GetTireLoad()/100;
		float MaxGrip = Tire->GetMaxGrip() / 100;
		float SlipRatio = Tire->GetSlipRatio();
		float SlipAngle = Tire->GetSlipAngle();
		float Fx = Tire->GetLastLongitudinalForce() / 100;
		float Fy = Tire->GetLastLateralForce()/100;
		float SuspensionForce = Tire->GetSuspensionForce()/100;
		float RollingResistance = RollingResistanceFunction(Tire)/100;
		FVector Contact = Tire->GetContactPoint()/100;
		DataLine += FString::Printf(TEXT(",%5.1f,%5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f"), Fz, MaxGrip, SlipRatio, SlipAngle, Fx, Fy, SuspensionForce, RollingResistance, Contact.X, Contact.Y, Contact.Z);
	}


	// Ensure the file ends with a newline
	if (!DataLine.EndsWith(TEXT("\n")))
	{
		DataLine.Append(TEXT("\n"));
	}

	// Write the line to the file

	AppendToFile(DataLine);
}

void TelemetryLogger::AppendToFile(const FString& Line)
{
	//Ensure the files exists before appending to it
	InitialiseLoggerCSV();


	FFileHelper::SaveStringToFile(Line, *FullFilePath,FFileHelper::EEncodingOptions::AutoDetect,&IFileManager::Get(), FILEWRITE_Append);
}

void TelemetryLogger::ChangeLoggerFile(const FName VehiclePresetName)
{
	CurrentLoggingFile = VehiclePresetName.ToString() + ".csv";
	FullFilePath = FPaths::ProjectSavedDir() / TEXT("Telemetry") / CurrentLoggingFile;
	//Ensure that there is a file to write to
	InitialiseLoggerCSV();
}

void TelemetryLogger::InitialiseLoggerCSV()
{
	// Create the file csv if  does not exist
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FullFilePath))
	{
		// Create header:
		FString Header = TEXT("Timestamp (s), Speed (m/s), AccelerationX(m/s^2), AccelerationY(m/s^2),Throttle, Brake, Steer, Drag (N), Pitch (degrees), Heave (m),");
		Header += TEXT("FR_Fz (N), FR_MaxGrip (N), FR_slipRatio, FR_SlipAngle (degrees), FR_Fx (N), FR_Fy (N), FR_SuspensionForce (N), FR_RollingResistance (N), FR_ContactX (m), FR_ContactY (m), FR_ContactZ (m),");
		Header += TEXT("FL_Fz (N), FL_MaxGrip (N), FL_slipRatio, FL_SlipAngle (degrees), FL_Fx (N), FL_Fy (N), FL_SuspensionForce (N), FL_RollingResistance (N), FL_ContactX (m), FL_ContactY (m), FL_ContactZ (m),");
		Header += TEXT("RR_Fz (N), RR_MaxGrip (N), RR_slipRatio, RR_SlipAngle (degrees), RR_Fx (N), RR_Fy (N), RR_SuspensionForce (N), RR_RollingResistance (N), RR_ContactX (m), RR_ContactY (m), RR_ContactZ (m),");
		Header += TEXT("RL_Fz (N), RL_MaxGrip (N), RL_slipRatio, RL_SlipAngle (degrees), RL_Fx (N), RL_Fy (N), RL_SuspensionForce (N), RL_RollingResistance (N), RL_ContactX (m), RL_ContactY (m), RL_ContactZ (m)\n");
		FFileHelper::SaveStringToFile(Header, *FullFilePath);
	}
}
