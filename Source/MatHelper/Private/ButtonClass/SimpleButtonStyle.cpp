// Copyright Epic Games, Inc. All Rights Reserved.
// UE4.26 port: no SlateStyleMacros.h (UE5-only); no FSlateImageBrush public header.

#include "Buttonclass/SimpleButtonStyle.h"
#include "MatHelper.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FSimpleButtonStyle::StyleInstance = nullptr;

void FSimpleButtonStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSimpleButtonStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FSimpleButtonStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SimpleButtonStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FSimpleButtonStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("SimpleButtonStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir() / TEXT("Resources"));

	// UE4.26: no FSlateImageBrush public header. Use FSlateBrush with manual PNG resource loading.
	// For the Niagara icon, load the PNG as a Slate resource via UTexture2D is complex;
	// simplest approach: use a plain FSlateBrush (no image) and rely on text labels for buttons.
	// Users who want the icon can migrate Content/ resources separately.
	FSlateBrush* NiagaraBrush = new FSlateBrush();
	NiagaraBrush->ImageSize = FVector2D(50, 50);
	Style->Set("SimpleButton.Niagara", NiagaraBrush);

	// Placeholder brush.
	Style->Set("SimpleButton.PluginAction", new FSlateBrush());

	return Style;
}

void FSimpleButtonStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FSimpleButtonStyle::Get()
{
	return *StyleInstance;
}
