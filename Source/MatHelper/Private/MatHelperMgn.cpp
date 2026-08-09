// Copyright AKaKLya 2024
// UE4.26 port: FEditorStyle instead of FAppStyle; no SVG brush; UAssetEditorSubsystem for open.

#include "MatHelperMgn.h"

#include "MatHelper.h"
#include "Interfaces/IPluginManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "EditorStyleSet.h"
#include "Materials/MaterialInstanceDynamic.h"

UMatHelperMgn::UMatHelperMgn()
{
	PluginButtonConfigPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir() + "/Config/AddNodeFile";
}

void UMatHelperMgn::ModifyICON()
{
	auto PluginPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir();

	// UE4.26: FEditorStyle::Get() returns ISlateStyle& (not FAppStyle).
	ISlateStyle& StyleRef = FEditorStyle::Get();
	FSlateStyleSet* Style = (FSlateStyleSet*)&StyleRef;

	FSlateBrush* MatIcon = CreateHeaderBrush();

	FString TheIConName = "Graph/" + IConName;
	Style->SetContentRoot(PluginPath + "/Resources/");
	if(IConName == "MatIcon")
	{
		Style->Set("AppIcon", MatIcon);
	}
	else
	{
		// UE4.26: no IMAGE_BRUSH_SVG / FSlateVectorImageBrush. Use a plain brush with the material resource.
		FSlateBrush* IconBrush = new FSlateBrush();
		IconBrush->ImageSize = FVector2D(50.f, 50.f);
		Style->Set("AppIcon", IconBrush);
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	AssetEditorSubsystem->CloseAllEditorsForAsset(this);
	AssetEditorSubsystem->OpenEditorForAsset(this);
}

FSlateBrush* UMatHelperMgn::CreateHeaderBrush()
{
	FSlateBrush* SlateBrush = new FSlateBrush();

	const FString MaterialPath = FString("/MatHelper/Material/M_SlateIcon");

	UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);

	DynamicMaterial->AddToRoot();

	SlateBrush->SetResourceObject(DynamicMaterial);
	SlateBrush->ImageSize = FVector2D(50.f, 50.f);
	return SlateBrush;
}

void UMatHelperMgn::OpenNodesConfigFolder()
{
	FString Path = PluginButtonConfigPath;
	Path.ReplaceCharInline('/','\\');
	FWindowsPlatformProcess::CreateProc(L"explorer.exe",*Path,false,false,false,nullptr,0,nullptr,nullptr,nullptr);
}


void UMatHelperMgn::EditButtonInfo()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FMatHelperModule::ButtonInfoEditorTabName);
}

void UMatHelperMgn::RefreshHelpersButton()
{
	FMatHelperModule::RefreshAllWidgetButton();
}

void UMatHelperMgn::RestartEditor()
{
	FUnrealEdMisc::Get().RestartEditor(false);
}
