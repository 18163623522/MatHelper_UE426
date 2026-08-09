// Copyright AKaKLya 2024
// UE4.26 port: replaces UAssetDefinition with FAssetTypeActions.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FCusAssetTypeActions_Material : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Material", "Material"); }
	virtual FColor GetTypeColor() const override { return MaterialAssetColor; }
	virtual UClass* GetSupportedClass() const override { return UMaterial::StaticClass(); }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::MaterialsAndTextures | EAssetTypeCategories::Basic; }
	virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const override;

	// Called by MatHelper module to apply user-configured color at runtime.
	void SetAssetColor(FColor InColor) { MaterialAssetColor = InColor; }

private:
	FColor MaterialAssetColor = FColor(255, 25, 25);
};
