// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CarSettings.h"
#include "CoreMinimal.h"

/**
 *
 */
class VEHICLESIMULATION_API VehiclePresets
{
public:
	VehiclePresets();
	~VehiclePresets();
	//Returns a stored preset
	CarSettings GetPreset(const int Index);

private:
	//Produces parameters for replicating the behaviour of a 2009 Chevrolet Corvette GT2
	void CreatePresetForChevroletCorvetteGT2();
	TArray <CarSettings> Presets;
};
