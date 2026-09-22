// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CarSettings.h"
#include "CoreMinimal.h"
#include "VehiclePresets.generated.h"


/**
 *The parameters for defining a vehicle
 */
enum DriveConfiguration
{
	FWD = 2, RWD = 3, AWD = 6
};
enum SteerConfiguration
{
	FWS = 2, RWS = 3, AWS = 6
};
enum  AXISPOSITION
{
	FRONT = 2, REAR = 3
};
USTRUCT(BlueprintType)
struct VEHICLESIMULATION_API FVehiclePresets
{
	GENERATED_BODY()
	FVehiclePresets() {
		CreatePresetForChevroletCorvetteGT2();
		CreatePresetForSkipBarber2000();
		CreatePresetForTruckSeriesTruck();
	}
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presets")
	TArray <FCarSettings> Presets;
	//Returns a stored preset
	FCarSettings GetPreset(const int Index) {
		if (Index < Presets.Num() && Index >= 0)
		{
			return Presets[Index];
		}
		//Return the default car settings
		return FCarSettings();
	}

	//Creates a wheel configuration base on an array of positions, a drive configuration, and a steering configuration
	TMap<FName, FWheelConfiguration> CreateWheelConfigurations(TArray<FVector> Positions, DriveConfiguration DriveConfig, SteerConfiguration SteerConfig)
	{
		TMap<FName, FWheelConfiguration> WheelConfigurations = { };
		for (int i = 0; i < SocketNames.Num(); i++)
		{
			FName CurrentSocket = SocketNames[i];
			FVector CurrentPosition = SocketNames.Num() == Positions.Num() ? Positions[i] : FVector(0, 0, 0);
			//Get the axis position of the wheel
			AXISPOSITION Axis = i < 2 ? AXISPOSITION::FRONT : AXISPOSITION::REAR;
			//
			WheelConfigurations.Add(CurrentSocket, FWheelConfiguration(CurrentPosition,
				DriveConfig % Axis == 0,
				i % 2 == 0,
				SteerConfig % Axis == 0,
				Axis == AXISPOSITION::REAR));
		}
		return WheelConfigurations;
	}

	TArray<FVector> CreateWheelPositions(const float X, const float Y, const float Z)
	{
		TArray<FVector> WheelPositions;
		for (int i = 0; i < 4; i++)
		{
			WheelPositions.Add(FVector(i < 2 ? X : -X, i % 2 == 0 ? Y : -Y, Z));
		}
		return WheelPositions;
	}

	//Produces parameters for replicating the behaviour of a 2009 Chevrolet Corvette GT2
	void CreatePresetForChevroletCorvetteGT2()
	{
		TMap<float, float> TorqueMap = { {0,-34}, {250,-15}, {500,9}, {750,84}, {1000,172}, {1250,240}, {1500,295}, {1750,360},
			{2000,405}, {2250,435}, {2500,490}, {2750,543}, {3000,598}, {3250,638}, {3500,664}, {3750,680}, {4000,710},
			{4250,722}, {4500,725}, {4750,703}, {5000,666}, {5250,628}, {5500,594}, {5750,562}, {6000,529}, {6250,499},
			{6500,468}, {6750,435}, {7000,403}, {7250,377}, {7500,340}, {7750,300}, {8000,260}, {8250,220}, {8500,180},
			{8750,140}, {9000,100}, {9250,50} };
		TMap<FName, FWheelConfiguration> NewWheelConfigurations = CreateWheelConfigurations(CreateWheelPositions(1.35f, 0.835f, 0.0f), DriveConfiguration::RWD, SteerConfiguration::FWS);
		TArray<MagicFormulaModel> TireFormulas = { {  21.2302f, 1.3424f, 0.1445f, 0.0f, 0.2106f,},{  25.7528f, 1.2348f, -2.268f, 0.0f, 0.2756f},{9.8933f,1.1722f,-5.0f,0.0341,-0.3648} };


		Presets.Add(FCarSettings("2009 Chevrolet Corvette GT2", 1190.6f, 1.163f, 2.1f,
			0.392f, 2.7f, 0.315f, 1.3365f,
			1.3635f, 1.67f, { 2.923f ,2.612f,2.357f,2.000f,1.875f,1.625f }, 3.800f,
			0.80f, 2090.0f, 392268.0f, 24900.0f,
			200512.0f, 191756.0f, 10500.0f, 9650.0f,
			1900.0f, 1920.0f, 40.03f, 43.83f,
			80000.0f, 81000.0f, 0.3252f, 0.3528f,
			2.0f, 2.09f, 350000.f, TorqueMap,
			250, 9250, 500, 0.0118,
			0.0133, 1.096f, 1.603f, 5234.4f, 3111.2f, TireFormulas, NewWheelConfigurations));
	}
	//Produces parameters for replicating the behaviour of a SkipBarber 2000
	void CreatePresetForSkipBarber2000()
	{
		TMap<float, float> TorqueMap = { {1000, 135 }, {1500,159}, {2000,173},{2500,176},
			{3000,181.2}, {3500,179},{4000,188}, {4500,191.4},
			{5000,192.4},{5500,187.2},{6000,173.3},{6500,157.7},
			{7000,142.5}};
		TMap<FName, FWheelConfiguration> NewWheelConfigurations = CreateWheelConfigurations(CreateWheelPositions(1.229f, 0.6668f, 0.0f), DriveConfiguration::RWD, SteerConfiguration::FWS);

		TArray<MagicFormulaModel> TireFormulas = { 
			{ 15.6386f, 1.5558f , 1.0504f,  0.0f , -0.0357f},
			{ 15.6386f, 1.5558f , 1.0504f,  0.0f , -0.0357f},
			{ 15.6386f, 1.5558f , 1.0504f,  0.0f , -0.0357f }
	};
		Presets.Add(FCarSettings("SkipBarber 2000", 501.446f, 1.0414f, 1.80f,
			0.356f, 2.458f, 0.311f, 0.973f,
			1.485f, 1.3525f, {2.067f,1.706f,1.444f,1.182,0.960f }, 3.444f,
			0.88f, 680.62f, 193515.0f, 15400.0f,
			7000.0f, 9250.0f, 7000.0f, 15000.0f,
			1440.0f,1450.0f, 63.2f, 63.854f,
			41406.0f, 50216.7f, 0.2957f, 0.31831f,
			1.28f, 1.29f, 90000.0f, TorqueMap,500.0f,
			7000, 1000, 0.0140,
			0.0150, 0.859f, 1.14882f, 1401.0f, 1401.0f, TireFormulas, NewWheelConfigurations));

	}


	//Produces parameters for replicating the behaviour of a 2017 Truck Series race truck
	void CreatePresetForTruckSeriesTruck()
	{
		TMap<float, float> TorqueMap = {
		{0,-62.3}, {50,-51.9}, {100,-41.6}, {150,-31.5}, {200,-21.4},
		{250,-11.3}, {300,-1.3}, {350,7.5}, {400,16.1}, {450,24.5},
		{500,32.9}, {550,41.2}, {600,49.6}, {650,57.7}, {700,65.9},
		{750,74}, {800,81.9}, {850,89.9}, {900,97.8}, {950,105.7},
		{1000,113.4}, {1050,121}, {1100,128.7}, {1150,136.3}, {1200,143.7},
		{1250,151.2}, {1300,158.5}, {1350,165.8}, {1400,173.1}, {1450,180.2},
		{1500,187.4}, {1550,194.3}, {1600,201.4}, {1650,208.3}, {1700,215.1},
		{1750,221.9}, {1800,228.6}, {1850,235.3}, {1900,241.8}, {1950,248.4},
		{2000,254.8}, {2050,261.2}, {2100,267.5}, {2150,273.8}, {2200,280},
		{2250,286.1}, {2300,292.2}, {2350,298.2}, {2400,304.1}, {2450,309.9},
		{2500,315.8}, {2550,321.5}, {2600,327.2}, {2650,332.8}, {2700,338.3},
		{2750,343.9}, {2800,349.2}, {2850,354.6}, {2900,359.8}, {2950,365},
		{3000,370.2}, {3050,375.2}, {3100,380.3}, {3150,385.3}, {3200,390.2},
		{3250,395}, {3300,399.7}, {3350,404.4}, {3400,409.1}, {3450,413.6},
		{3500,418.2}, {3550,422.5}, {3600,426.9}, {3650,431.2}, {3700,435.4},
		{3750,439.7}, {3800,443.7}, {3850,447.8}, {3900,451.7}, {3950,455.6},
		{4000,459.6}, {4050,463.3}, {4100,467}, {4150,470.7}, {4200,474.3},
		{4250,477.8}, {4300,481.2}, {4350,484.6}, {4400,488}, {4450,491.3},
		{4500,494.5}, {4550,497.5}, {4600,500.7}, {4650,503.7}, {4700,506.6},
		{4750,509.5}, {4800,512.3}, {4850,515.1}, {4900,517.7}, {4950,520.3},
		{5000,522.9}, {5050,525.3}, {5100,527.7}, {5150,530.1}, {5200,532.3},
		{5250,534.6}, {5300,536.7}, {5350,538.9}, {5400,540.8}, {5450,542.9},
		{5500,544.7}, {5550,546.6}, {5600,548.3}, {5650,550}, {5700,551.7},
		{5750,553.2}, {5800,554.8}, {5850,556.2}, {5900,557.6}, {5950,558.8},
		{6000,560.1}, {6050,561.3}, {6100,562.4}, {6150,563.4}, {6200,564.5},
		{6250,565.3}, {6300,566.2}, {6350,567}, {6400,567.7}, {6450,568.4},
		{6500,569}, {6550,569.5}, {6600,570}, {6650,570.3}, {6700,570.7},
		{6750,571}, {6800,571.2}, {6850,571.3}, {6900,571.3}, {6950,571.3},
		{7000,571.3}, {7050,571.2}, {7100,571}, {7150,570.8}, {7200,570.4},
		{7250,570.1}, {7300,569.6}, {7350,569}, {7400,568.5}, {7450,567.9},
		{7500,567.2}, {7550,566.4}, {7600,565.5}, {7650,564.7}, {7700,563.7},
		{7750,562.7}, {7800,561.5}, {7850,560.4}, {7900,559.2}, {7950,557.8},
		{8000,556.5}, {8050,555}, {8100,553.6}, {8150,552}, {8200,550.4},
		{8250,548.7}, {8300,546.9}, {8350,543.6}, {8400,540.2}, {8450,536.6},
		{8500,532.9}, {8550,529.1}, {8600,525}, {8650,520.8}, {8700,516.4},
		{8750,511.8}, {8800,507.1}, {8850,502.2}, {8900,497}, {8950,491.8},
		{9000,486.1}, {9050,480.4}, {9100,474.3}, {9150,468}, {9200,461.5},
		{9250,454.7}, {9300,447.6}, {9350,440.2}, {9400,432.5}, {9450,424.5},
		{9500,416.1}, {9550,407.5}, {9600,398.5}, {9650,389.1}, {9700,379.3},
		{9750,369.1}, {9800,358.6}, {9850,347.6}, {9900,336.1}, {9950,324},
		{10000,311.6} };
		TMap<FName, FWheelConfiguration> NewWheelConfigurations = CreateWheelConfigurations(CreateWheelPositions(1.345f, 0.781f, 0.0f), DriveConfiguration::RWD, SteerConfiguration::FWS);

		TArray<MagicFormulaModel> TireFormulas = {
			{13.7345f, 1.7189f, 0.7099f, 0.0014f, -0.05f},
			{13.7345f, 1.7189f, 0.7099f, 0.0014f, -0.05f},
			{12.981f, 1.7188f, 0.6924f, 0.0018f, -0.05f}
		};

		Presets.Add(FCarSettings("2017 Truck Series Truck", 1268.02f, 1.55f, 1.93f,
			0.269f, 2.69f, 0.318f, 1.2654f,
			1.4246f, 1.562f, { 2.5f, 1.6875f, 1.28f, 1.0f }, 5.6667f,
			0.88f, 2998.0f, 1003152.2f, 297920.9f,
			308576.1f, 193000.0f, 7442.0f, 10505.0f,
			800.0f, 800.0f, 48.944f, 81.2744f,
			73000.0f, 73000.0f, 0.35776f, 0.35776f,
			1.91f, 1.91f, 500000.0f, TorqueMap, 50.0f,
			9200, 1730, 0.0203f,
			0.0203f, 2.0443f, 2.0015f, 7365.0f, 3165.0f, TireFormulas, NewWheelConfigurations));
	}
};
