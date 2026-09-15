// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
struct VEHICLESIMULATION_API MagicFormulaModel
{
	//It dictates how quickly the tire builds up grip as slip 
	float StiffnessFactor = 0.0f;
	//Determines the overall shape of the curve 
	float ShapeFactors = 0.0f;
	//Determines how much grip is lost once the tire starts sliding 
	float CurvatureFactor = 0.0f;
	//Offset at the slip axis origin
	float HorizontalShift = 0.0f;
	//The Force coefficient at zero slip 
	float VerticalShift = 0.0f;
	float MagicFormula(const float peakValue, const float x) const
	{
		float StiffnessEffect = StiffnessFactor * x;
		float CurvatureEffect = CurvatureFactor * (StiffnessEffect - FMath::Atan(StiffnessEffect));
		float arc = FMath::Atan(StiffnessEffect - CurvatureEffect);
		return peakValue * FMath::Sin(ShapeFactors * arc);
	}

};