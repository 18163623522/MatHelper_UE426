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
class FMatHelperPaletteInjector;

// Factory defined in MatHelperWidget.cpp (needs full FMaterialEditor definition).
TSharedPtr<FMatHelperPaletteInjector> MakeMatHelperPaletteInjector(const TWeakPtr<IMaterialEditor>& InMatEditor);


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

	UMatHelperMgn* MatHelperMgn;

	// UE4.26: no C++17 inline static; declare in header, define in cpp.
	static const FName ButtonInfoEditorTabName;
	static const FName SceneViewEditorTabName;
	static const FName MaterialSceneViewEditorTabName;

	static void PlayNiagaraOnEditorWorld();

	// UE4.26: Palette is created after OnMaterialEditorOpened broadcasts, so injection is
	// deferred through tickable injectors until the Palette widget exists.
	void InjectPaletteWidget(FMaterialEditor* MatEditor);
	void RemovePaletteInjector(FMatHelperPaletteInjector* Injector);

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

	// UE4.26: AddDefaultSystemTracks removed (no OnNewActorTrackAdded in ILevelSequenceModule).
	FDelegateHandle MaterialOpenHandle;
	FDelegateHandle MaterialInstanceOpenHandle;

	// UE4.26: registered asset type actions (replace UE5 UAssetDefinition registry).
	TSharedPtr<FCusAssetTypeActions_Material> MaterialAssetTypeActions;
	TSharedPtr<FCusAssetTypeActions_MaterialInstanceConstant> MaterialInstanceAssetTypeActions;
	TSharedPtr<FCusAssetTypeActions_MaterialInterface> MaterialInterfaceAssetTypeActions;

	// UE4.26: pending palette injections waiting for Palette widget creation.
	TArray<TSharedPtr<FMatHelperPaletteInjector>> PendingInjectors;
};

