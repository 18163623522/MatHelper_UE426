// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "IMaterialEditor.h"


/**
 * V6.1: batch enable/disable of all MIC parameters.
 * UE4.26: static entry point so the material instance editor toolbar can call it
 * directly (the widget itself had no instantiation point in the original code).
 */

class UMaterialInstanceConstant;

class SMatInstanceHelper : public SScrollBox
{
public:
	SLATE_BEGIN_ARGS(SMatInstanceHelper) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs,TSharedPtr<UMaterialInstanceConstant> InMatInstanceConstant);

	// UE4.26: static entry for toolbar buttons in the material instance editor.
	static void ToggleAllParams(UMaterialInstanceConstant* InMatInstanceConstant, bool bIsOpen);

private:
	TSharedPtr<UMaterialInstanceConstant> MatInstanceConstant;
	FReply ToogleParams(bool bIsOpen);
};
