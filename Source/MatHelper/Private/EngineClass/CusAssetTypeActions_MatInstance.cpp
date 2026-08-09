// Copyright AKaKLya 2024
// UE4.26 port: replaces UAssetDefinition with FAssetTypeActions.
// Material Instance Constant asset type actions — opens the material instance editor.

#include "EngineClass/CusAssetTypeActions_MatInstance.h"

#include "IMaterialEditor.h"
#include "MaterialEditorModule.h"
#include "ThumbnailRendering/SceneThumbnailInfoWithPrimitive.h"
#include "Materials/MaterialInstanceConstant.h"

void FCusAssetTypeActions_MaterialInstanceConstant::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(Obj))
		{
			IMaterialEditorModule& MaterialEditorModule = FModuleManager::LoadModuleChecked<IMaterialEditorModule>("MaterialEditor");
			// 4.26 signature: CreateMaterialInstanceEditor(Mode, Host, UMaterialInstance*)
			MaterialEditorModule.CreateMaterialInstanceEditor(EToolkitMode::Standalone, EditWithinLevelEditor, MIC);
		}
	}
}

UThumbnailInfo* FCusAssetTypeActions_MaterialInstanceConstant::GetThumbnailInfo(UObject* Asset) const
{
	if (UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(Asset))
	{
		if (USceneThumbnailInfoWithPrimitive* ThumbnailInfo = Cast<USceneThumbnailInfoWithPrimitive>(MaterialInterface->ThumbnailInfo))
		{
			return ThumbnailInfo;
		}
		USceneThumbnailInfoWithPrimitive* NewThumbnailInfo = NewObject<USceneThumbnailInfoWithPrimitive>(MaterialInterface, NAME_None, RF_Transactional);
		MaterialInterface->ThumbnailInfo = NewThumbnailInfo;
		return NewThumbnailInfo;
	}
	return nullptr;
}
