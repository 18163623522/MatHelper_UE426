// Copyright AKaKLya 2024
// UE4.26 port: replaces UAssetDefinition with FAssetTypeActions.

#include "EngineClass/CusAssetTypeActions_MatInterface.h"

#include "ThumbnailRendering/SceneThumbnailInfoWithPrimitive.h"
#include "Materials/MaterialInterface.h"

UThumbnailInfo* FCusAssetTypeActions_MaterialInterface::GetThumbnailInfo(UObject* Asset) const
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
