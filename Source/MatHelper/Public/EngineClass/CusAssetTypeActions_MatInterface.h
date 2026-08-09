// Copyright AKaKLya 2024
// UE4.26 port: replaces UAssetDefinition with FAssetTypeActions.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FCusAssetTypeActions_MaterialInterface : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MaterialInterface", "Material Interface"); }
	virtual FColor GetTypeColor() const override { return FColor(64, 192, 64); }
	virtual UClass* GetSupportedClass() const override { return UMaterialInterface::StaticClass(); }
	virtual uint32 GetCategories() override { return EAssetTypeCategories::MaterialsAndTextures; }
	virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const override;
};
