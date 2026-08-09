// Copyright AKaKLya 2024
// UE4.26 port: replaces UAssetDefinition with FAssetTypeActions.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "Materials/MaterialInstanceConstant.h"

class FCusAssetTypeActions_MaterialInstanceConstant : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MaterialInstanceConstant", "Material Instance"); }
	virtual FColor GetTypeColor() const override { return MaterialInstanceAssetColor; }
	virtual UClass* GetSupportedClass() const override { return UMaterialInstanceConstant::StaticClass(); }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::MaterialsAndTextures; }
	virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const override;
	virtual bool IsImportedAsset() const override { return true; }

	// Called by MatHelper module to apply user-configured color at runtime.
	void SetAssetColor(FColor InColor) { MaterialInstanceAssetColor = InColor; }

private:
	FColor MaterialInstanceAssetColor = FColor(0, 128, 0);
};
