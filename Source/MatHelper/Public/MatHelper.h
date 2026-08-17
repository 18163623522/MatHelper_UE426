// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"

class ISceneOutliner;
class ISceneOutlinerColumn;
class ISequencer;
class FCusAssetTypeActions_Material;
class FCusAssetTypeActions_MaterialInstanceConstant;
class FCusAssetTypeActions_MaterialInterface;
class UMatHelperMgn;
class SMatHelperWidget;
class IMaterialEditor;
class FMaterialEditor;


class FMatHelperModule : public IModuleInterface
{
public:
	static FMatHelperModule& Get();
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void EditorNotify(const FString&  NotifyInfo, SNotificationItem::ECompletionState State);
	static void RefreshAllWidgetButton();

	FString GetPluginPath() {return PluginPath;};

	TSharedRef<SDockTab> OnSpawnButtonInfoEditor(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawnSceneEditorView(const FSpawnTabArgs& SpawnTabArgs);

	// UE4.26 additions: extra content browser windows + a texture picker for materials.
	TSharedRef<SDockTab> OnSpawnContentBrowser(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawnTextureBrowser(const FSpawnTabArgs& SpawnTabArgs);

	UMatHelperMgn* MatHelperMgn;

	// UE4.26: no C++17 inline static; declare in header, define in cpp.
	static const FName ButtonInfoEditorTabName;
	static const FName SceneViewEditorTabName;
	static const FName MaterialSceneViewEditorTabName;
	static const FName TextureBrowserTabName;

	static void PlayNiagaraOnEditorWorld();

	// UE4.26: Palette is created after OnMaterialEditorOpened broadcasts, so injection is
	// deferred via a core-ticker poll until the Palette widget exists.
	void InjectPaletteWidget(FMaterialEditor* MatEditor);

private:
	FString PluginPath;
	TSharedPtr<class FUICommandList> PlayNiagaraCommands;

	void RegisterTab();
	void RegisterButton();
	void RegisterNiagaraAutoPlayer();
	void ToggleAssetFlag(bool bIsLock);
	void NiagaraToolBarExtend(FToolBarBuilder& ToolbarBuilder);

	void InitMatEditorHook();
	void InitNiagaraEditorHook();
	void InitPluginInfo();

	// UE4.26: persists the rebuilt config DataAsset once the asset registry scan
	// is out of the way (deferred via a retrying core ticker; never during startup).
	// Returns true to keep retrying on the next tick.
	bool DeferredSaveConfigAsset();

	// UE4.26: after a failed UE5-format load, force the class defaults back into
	// the rebuilt object (NewObject can pick up half-initialized leftovers).
	void RestoreConfigDefaults(UMatHelperMgn* Mgn);

	// UE4.26: AddDefaultSystemTracks removed (no OnNewActorTrackAdded in ILevelSequenceModule).
	FDelegateHandle MaterialOpenHandle;
	FDelegateHandle MaterialInstanceOpenHandle;

	// UE4.26: registered asset type actions (replace UE5 UAssetDefinition registry).
	TSharedPtr<FCusAssetTypeActions_Material> MaterialAssetTypeActions;
	TSharedPtr<FCusAssetTypeActions_MaterialInstanceConstant> MaterialInstanceAssetTypeActions;
	TSharedPtr<FCusAssetTypeActions_MaterialInterface> MaterialInterfaceAssetTypeActions;
};

