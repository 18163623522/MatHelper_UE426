// Copyright AKaKLya 2024
// UE4.26 port: replaces UAssetDefinition with FAssetTypeActions.

#include "EngineClass/CusAssetTypeActions_Material.h"

#include "IMaterialEditor.h"
#include "MaterialEditorModule.h"
#include "ThumbnailRendering/SceneThumbnailInfoWithPrimitive.h"
#include "Materials/Material.h"

void FCusAssetTypeActions_Material::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		if (UMaterial* Material = Cast<UMaterial>(Obj))
		{
			IMaterialEditorModule& MaterialEditorModule = FModuleManager::LoadModuleChecked<IMaterialEditorModule>("MaterialEditor");
			MaterialEditorModule.CreateMaterialEditor(EToolkitMode::Standalone, EditWithinLevelEditor, Material);
		}
	}
}

UThumbnailInfo* FCusAssetTypeActions_Material::GetThumbnailInfo(UObject* Asset) const
{
	if (UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(Asset))
	{
		// UE4.26 pattern: manually create thumbnail info if missing (no UE::Editor::FindOrCreateThumbnailInfo).
		if (USceneThumbnailInfoWithPrimitive* ThumbnailInfo = Cast<USceneThumbnailInfoWithPrimitive>(MaterialInterface->ThumbnailInfo))
		{
			const UMaterial* Material = MaterialInterface->GetBaseMaterial();
			if (Material && Material->bUsedWithParticleSprites)
			{
				ThumbnailInfo->PrimitiveType = TPT_Plane;
			}
			return ThumbnailInfo;
		}
		// Create new if none exists.
		USceneThumbnailInfoWithPrimitive* NewThumbnailInfo = NewObject<USceneThumbnailInfoWithPrimitive>(MaterialInterface, NAME_None, RF_Transactional);
		MaterialInterface->ThumbnailInfo = NewThumbnailInfo;

		const UMaterial* Material = MaterialInterface->GetBaseMaterial();
		if (Material && Material->bUsedWithParticleSprites)
		{
			NewThumbnailInfo->PrimitiveType = TPT_Plane;
		}
		return NewThumbnailInfo;
	}
	return nullptr;
}
