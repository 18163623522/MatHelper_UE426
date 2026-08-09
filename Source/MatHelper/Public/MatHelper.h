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
};

