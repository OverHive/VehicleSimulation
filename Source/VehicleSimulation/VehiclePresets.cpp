// Fill out your copyright notice in the Description page of Project Settings.


#include "VehiclePresets.h"

VehiclePresets::VehiclePresets()
{
	CreatePresetForChevroletCorvetteGT2();
}
CarSettings VehiclePresets::GetPreset(const int Index)
{
	if (Index < Presets.Num() && Index >= 0)
	{
		return Presets[Index];
	}
	//Return the default car settings
	return CarSettings();
}
VehiclePresets::~VehiclePresets()
{
}
void VehiclePresets::CreatePresetForChevroletCorvetteGT2()
{
	TMap<float, float> TorqueMap = { {0,-34}, {250,-15}, {500,9}, {750,84}, {1000,172}, {1250,240}, {1500,295}, {1750,360},
		{2000,405}, {2250,435}, {2500,490}, {2750,543}, {3000,598}, {3250,638}, {3500,664}, {3750,680}, {4000,710},
		{4250,722}, {4500,725}, {4750,703}, {5000,666}, {5250,628}, {5500,594}, {5750,562}, {6000,529}, {6250,499},
		{6500,468}, {6750,435}, {7000,403}, {7250,377}, {7500,340}, {7750,300}, {8000,260}, {8250,220}, {8500,180},
		{8750,140}, {9000,100}, {9250,50} };
	Presets.Add(CarSettings("2009 Chevrolet Corvette GT2", 1190.6f, 116.3f, 2.1f,
		0.392f, 2.7f, 0.315f, 1.359f,
		1.361f, 1.67f, { 1.625f,1.875f,2.000f,2.357f,2.612f,2.923f }, 3.800f,
		0.80f, 2090.0f, 392268.0f, 24900.0f,
		200512.0f, 191756.0f, 10500.0f, 9650.0f,
		1900.0f, 1920.0f, 40.03f, 43.83f,
		80000.0f, 81000.0f, 32.52f, 35.28f,
		2.0f, 2.09f, 350000.f, TorqueMap,
		250, 9250, 500, 0.0118,
		0.0133, 1.096f, 1.603f, 5234.4f, 3111.2f));
}

void VehiclePresets::CreatePresetForSkipBarber2000()
{
	//0.014, 0.0.15
}
