// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
struct VEHICLESIMULATION_API MagicFormulaModel
{
	//It dictates how quickly the tire builds up grip as slip 
	float StiffnessFactor = 0.0f;
	//Determines the overall shape of the curve 
	float ShapeFactor = 0.0f;
	//Determines how much grip is lost once the tire starts sliding 
	float CurvatureFactor = 0.0f;
	//Offset at the slip axis origin
	float HorizontalShift = 0.0f;
	//The force coefficient at zero slip 
	float VerticalShift = 0.0f;
	//The force coefficient at the peak
	float PeakFactor = 0.0f;
	MagicFormulaModel() {};
	MagicFormulaModel(const float NewPeakFactor, const float NewStiffnessFactor, const float NewShapeFactor, const float NewCurvatureFactor, const float NewHorizontalShift, const float NewVerticalShift)
	{
		PeakFactor = NewPeakFactor;
		StiffnessFactor = NewStiffnessFactor;
		ShapeFactor = NewShapeFactor;
		CurvatureFactor = NewCurvatureFactor;
		HorizontalShift = NewHorizontalShift;
		VerticalShift = NewVerticalShift;
	}
	float MagicFormula(const float peakValue, const float x) const
	{
		float Input = (HorizontalShift + x);
		float StiffnessEffect = StiffnessFactor * Input;
		float CurvatureEffect = CurvatureFactor * (StiffnessEffect - FMath::Atan(StiffnessEffect));
		float arc = FMath::Atan(StiffnessEffect - CurvatureEffect);
		return peakValue *FMath::Clamp(PeakFactor *FMath::Sin(ShapeFactor * arc) + VerticalShift,-1,1);
	}

};