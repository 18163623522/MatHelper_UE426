
#pragma execution_character_set("utf-8")
// Copyright AKaKLya 2024
// UE4.26 port: FAssetTypeActions instead of UAssetDefinition; FEditorStyle instead of FAppStyle.

#include "MatHelper.h"

#include "Engine/DeveloperSettings.h"
#include "MaterialInstanceEditor.h"
#include "TAccessPrivate.inl"
#include "AssetSelection.h"
#include "ButtonInfoEditor.h"
#include "MatHelperMgn.h"
#include "MatHelperWidget.h"
#include "IMaterialEditor.h"
#include "MaterialEditorModule.h"
#include "EngineClass/CusAssetTypeActions_Material.h"

// UE4.26: use #define hack to access private/protected members (Palette, ChildSlot).
// This is a common UE plugin technique — restore immediately after includes.
#define private public
#define protected public
#include "MaterialEditor.h"
#include "SMaterialPalette.h"
#undef private
#undef protected

#include "MatHelperSettings.h"
#include "MatInstanceHelper.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "TextureBrowser.h"
#include "WorkspaceMenuStructureModule.h"
#include "WorkspaceMenuStructure.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCube.h"
#include "MovieScene.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraEditorModule.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemEditorData.h"
#include "OuterlineSelectionCol.h"
#include "SceneEditorView.h"
#include "SceneOutlinerModule.h"
#include "ButtonClass/SimpleButtonCommands.h"
#include "Editor/Sequencer/Public/ISequencer.h"
#include "EngineClass/CusAssetTypeActions_MatInstance.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "MovieScene/MovieSceneNiagaraSystemSpawnSection.h"
#include "MovieScene/MovieSceneNiagaraSystemTrack.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "EditorStyleSet.h"
#include "ToolMenus.h"
#include <type_traits>

// UE4.26: AddDefaultSystemTracks removed — no need for AccessNiagaraAgeUpdateMode template hack.

// UE4.26: accessor class to expose protected SCompoundWidget::ChildSlot.
class FCompoundWidgetAccessor : public SCompoundWidget
{
public:
	using SCompoundWidget::ChildSlot;
};

#define LOCTEXT_NAMESPACE "FMatHelperModule"

// UE4.26: no C++17 inline static — define static members here.
const FName FMatHelperModule::ButtonInfoEditorTabName = "ButtonInfoEditor";
const FName FMatHelperModule::SceneViewEditorTabName = "SceneEditorView";
const FName FMatHelperModule::MaterialSceneViewEditorTabName = "MaterialSceneEditorView";
const FName FMatHelperModule::TextureBrowserTabName = "MatHelperTextureBrowser";

namespace MatHelperSpace
{
	TArray<TWeakPtr<SMatHelperWidget>> MhWidgets;

	const FName SceneViewEditorTabName1 = "SceneEditorView1";
	const FName SceneViewEditorTabName2 = "SceneEditorView2";
	const FName SceneViewEditorTabName3 = "SceneEditorView3";
	const FName SceneViewEditorTabName4 = "SceneEditorView4";
	const FName SceneViewEditorTabName5 = "SceneEditorView5";
	const FName SceneViewEditorTabName6 = "SceneEditorView6";
	const FName SceneViewEditorTabName7 = "SceneEditorView7";
	const FName SceneViewEditorTabName8 = "SceneEditorView8";
	const FName SceneViewEditorTabName9 = "SceneEditorView9";

	// UE4.26 addition: extra plugin-scoped content browser instances (registered
	// into the native Content Browser workspace group; native count is 4, so these
	// extend the set to 15 total).
	const FName ContentBrowserTabName1 = "MatHelperContentBrowser1";
	const FName ContentBrowserTabName2 = "MatHelperContentBrowser2";
	const FName ContentBrowserTabName3 = "MatHelperContentBrowser3";
	const FName ContentBrowserTabName4 = "MatHelperContentBrowser4";
	const FName ContentBrowserTabName5 = "MatHelperContentBrowser5";
	const FName ContentBrowserTabName6 = "MatHelperContentBrowser6";
	const FName ContentBrowserTabName7 = "MatHelperContentBrowser7";
	const FName ContentBrowserTabName8 = "MatHelperContentBrowser8";
	const FName ContentBrowserTabName9 = "MatHelperContentBrowser9";
	const FName ContentBrowserTabName10 = "MatHelperContentBrowser10";
	const FName ContentBrowserTabName11 = "MatHelperContentBrowser11";
	const FName MaterialInstanceSceneViewEditorTabName = "MaterialInstanceSceneEditorView";
	const FName NiagaraSceneViewEditorTabName = "NiagaraSceneViewEditorTabName";
	TSharedRef<ISceneOutlinerColumn> OnCreateOutlinerColumn(ISceneOutliner& SceneOutliner);

	static bool HasPlayWorld();
	static bool HasNoPlayWorld();
	static bool CanShowCommonMaps();
	static void OpenCommonMap_Clicked(const FString MapPath);
	static TSharedRef<SWidget> GetCommonMapsDropdown();
	static void RegisterGameEditorMenus();
}
using namespace MatHelperSpace;

FMatHelperModule& FMatHelperModule::Get()
{
	return FModuleManager::GetModuleChecked<FMatHelperModule>("MatHelper");
}

void FMatHelperModule::StartupModule()
{
	InitPluginInfo();
	RegisterTab();
	InitMatEditorHook();
	InitNiagaraEditorHook();
	RegisterGameEditorMenus();
}

void FMatHelperModule::ShutdownModule()
{
	// UE4.26: ILevelSequenceModule has no OnNewActorTrackAdded delegate — Niagara auto-track removed.
	if (FModuleManager::Get().IsModuleLoaded("MaterialEditor"))
	{
		IMaterialEditorModule& MatInterface = IMaterialEditorModule::Get();
		MatInterface.OnMaterialEditorOpened().Remove(MaterialOpenHandle);
		MatInterface.OnMaterialInstanceEditorOpened().Remove(MaterialOpenHandle);
	}

	// UE4.26: unregister our asset type actions.
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		if (MaterialAssetTypeActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(MaterialAssetTypeActions.ToSharedRef());
		}
		if (MaterialInstanceAssetTypeActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(MaterialInstanceAssetTypeActions.ToSharedRef());
		}
		if (MaterialInterfaceAssetTypeActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(MaterialInterfaceAssetTypeActions.ToSharedRef());
		}
	}
}

void FMatHelperModule::InitPluginInfo()
{
	PluginPath = IPluginManager::Get().FindPlugin("MatHelper")->GetBaseDir();
	MatHelperMgn = LoadObject<UMatHelperMgn>(nullptr,TEXT("/MatHelper/MatHelper.MatHelper"));

	// UE4.26: Content uasset may be UE5 format and fail to load.
	// Rebuild the config object in memory now, and persist it to disk later:
	// saving during StartupModule races the asset registry's plugin-content scan,
	// and SavePackage fatals the editor when the old file cannot be replaced.
	if (!MatHelperMgn)
	{
		UE_LOG(LogTemp, Warning, TEXT("MatHelper: Failed to load /MatHelper/MatHelper.MatHelper — rebuilding the DataAsset (deferred save to disk)."));
		UPackage* ConfigPackage = CreatePackage(nullptr, TEXT("/MatHelper/MatHelper"));
		MatHelperMgn = NewObject<UMatHelperMgn>(ConfigPackage, TEXT("MatHelper"), RF_Public | RF_Standalone);
		// The failed UE5-format load can leave this package with half-initialized
		// leftovers that NewObject may pick up (empty arrays, defaults missing) —
		// force the class defaults back in so the panel has working data.
		RestoreConfigDefaults(MatHelperMgn);
		FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			// Keep ticking (return true) while the old file is still locked by the
			// asset registry scan; stop once saved or permanently failed.
			return FMatHelperModule::Get().DeferredSaveConfigAsset();
		}), 10.0f);
	}

	FSimpleButtonStyle::Initialize();
	FSimpleButtonStyle::ReloadTextures();
	FSimpleButtonCommands::Register();

	PlayNiagaraCommands = MakeShareable(new FUICommandList);
	PlayNiagaraCommands->MapAction(
		FSimpleButtonCommands::Get().PlayNiagaraAction,
		FExecuteAction::CreateStatic(&PlayNiagaraOnEditorWorld),
		FCanExecuteAction());
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMatHelperModule::RegisterButton));
}

void FMatHelperModule::RestoreConfigDefaults(UMatHelperMgn* Mgn)
{
	const UMatHelperMgn* CDO = GetDefault<UMatHelperMgn>();
	Mgn->SceneViewMethod = CDO->SceneViewMethod;
	Mgn->MaterialAssetColor = CDO->MaterialAssetColor;
	Mgn->MaterialInstanceAssetColor = CDO->MaterialInstanceAssetColor;
	Mgn->HeightRatio = CDO->HeightRatio;
	Mgn->RootOffset = CDO->RootOffset;
	Mgn->BaseOffset = CDO->BaseOffset;
	Mgn->MaskPinInfo = CDO->MaskPinInfo;
	Mgn->AutoGroupKeys = CDO->AutoGroupKeys;
	Mgn->NodeButtonInfo = CDO->NodeButtonInfo;
	Mgn->IConName = CDO->IConName;
	Mgn->OverrideNiagaraSequenceMode = CDO->OverrideNiagaraSequenceMode;
	Mgn->CreateNiagaraAutoPlaySelection = CDO->CreateNiagaraAutoPlaySelection;
}

bool FMatHelperModule::DeferredSaveConfigAsset()
{
	if (!MatHelperMgn)
	{
		return false;
	}
	const FString SaveFile = PluginPath / TEXT("Content/MatHelper.uasset");

	// Move the old (UE5-format) file aside instead of deleting it: the asset
	// registry scanner opens package files WITHOUT FILE_SHARE_DELETE, so Delete
	// fails while a rename succeeds.
	if (IFileManager::Get().FileExists(*SaveFile))
	{
		FString BackupFile = SaveFile + TEXT(".ue5bak");
		int32 BackupIndex = 0;
		while (IFileManager::Get().FileExists(*BackupFile))
		{
			BackupFile = SaveFile + FString::Printf(TEXT(".ue5bak%d"), ++BackupIndex);
		}
		if (!IFileManager::Get().Move(*BackupFile, *SaveFile, /*bReplace*/ true))
		{
			UE_LOG(LogTemp, Verbose, TEXT("MatHelper: old MatHelper.uasset is still locked — retrying in 10s."));
			return true;
		}
	}

	UPackage* ConfigPackage = MatHelperMgn->GetOutermost();
	if (UPackage::SavePackage(ConfigPackage, MatHelperMgn, RF_Public | RF_Standalone, *SaveFile))
	{
		UE_LOG(LogTemp, Log, TEXT("MatHelper: rebuilt 4.26 DataAsset at %s"), *SaveFile);
		return false;
	}
	UE_LOG(LogTemp, Warning, TEXT("MatHelper: deferred save of the rebuilt DataAsset failed — will retry on the next editor launch."));
	return false;
}


void FMatHelperModule::RegisterTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ButtonInfoEditorTabName, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnButtonInfoEditor))
		.SetDisplayName(FText::FromString(L"\u8282\u70b9\u7f16\u8f91\u5668"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	///////////////////////////////////////////////////////////////////
	auto RegisterSceneView = [&](const FName& ID,const FString& Name)
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ID, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnSceneEditorView))
		.SetDisplayName(FText::FromString(Name))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	};

	RegisterSceneView(SceneViewEditorTabName,"SceneView");
	RegisterSceneView(MaterialSceneViewEditorTabName,"SceneView");
	RegisterSceneView(MaterialInstanceSceneViewEditorTabName,"SceneView");
	RegisterSceneView(NiagaraSceneViewEditorTabName,"SceneView");

	///////////////////////////////////////////////////////////////////
	RegisterSceneView(SceneViewEditorTabName1,"SceneView 1");
	RegisterSceneView(SceneViewEditorTabName2,"SceneView 2");
	RegisterSceneView(SceneViewEditorTabName3,"SceneView 3");
	RegisterSceneView(SceneViewEditorTabName4,"SceneView 4");
	RegisterSceneView(SceneViewEditorTabName5,"SceneView 5");
	RegisterSceneView(SceneViewEditorTabName6,"SceneView 6");
	RegisterSceneView(SceneViewEditorTabName7,"SceneView 7");
	RegisterSceneView(SceneViewEditorTabName8,"SceneView 8");
	RegisterSceneView(SceneViewEditorTabName9,"SceneView 9");
	///////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////
	// UE4.26: register 11 extra browsers INTO the native "Content Browser" workspace
	// group (Window menu), so they sit alongside the engine's built-in four.
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();
	TSharedRef<FWorkspaceItem> ContentBrowserGroup = MenuStructure.GetToolsCategory()->AddGroup(
		FText::FromString(L"Content Browser"),
		FText::FromString(L"Open a Content Browser tab."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.TabIcon"),
		true);
	const FName ContentBrowserTabs[11] =
	{
		ContentBrowserTabName1, ContentBrowserTabName2, ContentBrowserTabName3,
		ContentBrowserTabName4, ContentBrowserTabName5, ContentBrowserTabName6,
		ContentBrowserTabName7, ContentBrowserTabName8, ContentBrowserTabName9,
		ContentBrowserTabName10, ContentBrowserTabName11
	};
	for (int32 CBIndex = 0; CBIndex < 11; ++CBIndex)
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ContentBrowserTabs[CBIndex], FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnContentBrowser))
			.SetDisplayName(FText::FromString(FString::Printf(L"\u5185\u5bb9\u6d4f\u89c8\u5668 %d", CBIndex + 5)))
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.TabIcon"))
			.SetGroup(ContentBrowserGroup)
			.SetMenuType(ETabSpawnerMenuType::Enabled);
	}

	// UE4.26 addition: material-focused texture browser (drag textures into the graph).
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TextureBrowserTabName, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnTextureBrowser))
		.SetDisplayName(FText::FromString(L"\u8d34\u56fe\u6d4f\u89c8\u5668"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.ContentBrowser"))
		.SetMenuType(ETabSpawnerMenuType::Enabled);
	///////////////////////////////////////////////////////////////////
}


void FMatHelperModule::InitMatEditorHook()
{
	// UE4.26: register custom asset type actions to override asset colors.
	// We register our own FAssetTypeActions which take priority for color display.
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	MaterialAssetTypeActions = MakeShareable(new FCusAssetTypeActions_Material());
	MaterialAssetTypeActions->SetAssetColor(MatHelperMgn->MaterialAssetColor);
	AssetTools.RegisterAssetTypeActions(MaterialAssetTypeActions.ToSharedRef());

	MaterialInstanceAssetTypeActions = MakeShareable(new FCusAssetTypeActions_MaterialInstanceConstant());
	MaterialInstanceAssetTypeActions->SetAssetColor(MatHelperMgn->MaterialInstanceAssetColor);
	AssetTools.RegisterAssetTypeActions(MaterialInstanceAssetTypeActions.ToSharedRef());

	MaterialInterfaceAssetTypeActions = MakeShareable(new FCusAssetTypeActions_MaterialInterface());
	AssetTools.RegisterAssetTypeActions(MaterialInterfaceAssetTypeActions.ToSharedRef());

	IMaterialEditorModule& MatInterface = IMaterialEditorModule::Get();

	MaterialOpenHandle = MatInterface.OnMaterialEditorOpened().AddLambda([&](const TWeakPtr<IMaterialEditor>& InMatEditor)
		{
			// UE4.26: the delegate fires BEFORE InitMaterialEditor runs — the Palette widget
			// does not exist yet at this point. Poll with a core ticker until it exists;
			// returning false removes the ticker safely.
			TWeakPtr<FMaterialEditor> WeakMatEditor = StaticCastSharedPtr<FMaterialEditor>(InMatEditor.Pin());
			FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMatEditor](float)
			{
				TSharedPtr<FMaterialEditor> MatEditor = WeakMatEditor.Pin();

				// Editor closed before Palette was created — give up.
				if (!MatEditor.IsValid())
				{
					return false;
				}

				// Palette is created late in InitMaterialEditor — wait for it.
				if (MatEditor->Palette.IsValid())
				{
					FMatHelperModule::Get().InjectPaletteWidget(MatEditor.Get());
					return false;
				}
				return true;
			}));
		});

	MaterialInstanceOpenHandle = MatInterface.OnMaterialInstanceEditorOpened().AddLambda([&](TWeakPtr<IMaterialEditor> InMatInstanceEditor)
	{
		if(InMatInstanceEditor.Pin())
		{
			IMaterialEditor* Editor = InMatInstanceEditor.Pin().Get();
			FMaterialInstanceEditor* MatInstanceEditor = static_cast<FMaterialInstanceEditor*>(Editor);

			Editor->OnRegisterTabSpawners().AddLambda([this](const TSharedRef<class FTabManager>& TabManager)
			{
				TabManager->RegisterTabSpawner(MaterialInstanceSceneViewEditorTabName, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnSceneEditorView))
					.SetDisplayName(FText::FromString(L"\u573a\u666f\u89c6\u56fe"))
					.SetGroup(TabManager->GetLocalWorkspaceMenuRoot())
					.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));
			});

			//Hook SceneView Button
			TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
			ToolbarExtender->AddToolBarExtension("Stats",EExtensionHook::After,nullptr,FToolBarExtensionDelegate::CreateLambda([=](FToolBarBuilder& ToolbarBuilder)
			{
				ToolbarBuilder.BeginSection(TEXT("MatHelper"));
				{
					ToolbarBuilder.AddToolBarButton(
					FUIAction(FExecuteAction::CreateLambda([&]()
					{
						MatInstanceEditor->GetTabManager()->TryInvokeTab(MaterialInstanceSceneViewEditorTabName);
					})),
					FName(TEXT("SceneView")),
					FText::FromString(L"\u573a\u666f\u89c6\u56fe"),
					FText::FromString(L"\u573a\u666f\u89c6\u56fe"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					EUserInterfaceActionType::Button
					);

					// V6.1: batch enable/disable all MIC parameters.
					UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(MatInstanceEditor->GetMaterialInterface());
					ToolbarBuilder.AddToolBarButton(
					FUIAction(FExecuteAction::CreateLambda([MIC]()
					{
						SMatInstanceHelper::ToggleAllParams(MIC, true);
					})),
					FName(TEXT("EnableAllParams")),
					FText::FromString(L"\u5f00\u542f\u6240\u6709\u53c2\u6570"),
					FText::FromString(L"\u5f00\u542f\u6240\u6709\u53c2\u6570"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					EUserInterfaceActionType::Button
					);

					ToolbarBuilder.AddToolBarButton(
					FUIAction(FExecuteAction::CreateLambda([MIC]()
					{
						SMatInstanceHelper::ToggleAllParams(MIC, false);
					})),
					FName(TEXT("DisableAllParams")),
					FText::FromString(L"\u5173\u95ed\u6240\u6709\u53c2\u6570"),
					FText::FromString(L"\u5173\u95ed\u6240\u6709\u53c2\u6570"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					EUserInterfaceActionType::Button
					);
				}
				ToolbarBuilder.EndSection();
			}));
			auto Mana = MatInstanceEditor->GetToolBarExtensibilityManager();
			Mana->AddExtender(ToolbarExtender);
		}
	});

}

void FMatHelperModule::InjectPaletteWidget(FMaterialEditor* MatEditor)
{
	if (!MatEditor || !MatEditor->Palette.IsValid())
	{
		return;
	}

	// 在原有控件的基础上 添加自定义的控件.
	auto MhWidget = SNew(SMatHelperWidget, MatEditor);
	MhWidgets.RemoveAll([](auto& WeakWidget){ return !WeakWidget.IsValid(); });
	MhWidgets.Add(MhWidget);

	// SCompoundWidget::ChildSlot is protected — read original content via GetChildren(),
	// then write through the layout-compatible FCompoundWidgetAccessor derived class.
	TSharedRef<SMaterialPalette> PaletteRef = MatEditor->Palette.ToSharedRef();
	FChildren* Children = PaletteRef->GetChildren();
	TSharedRef<SWidget> OriginalContent = Children->GetChildAt(0);

	FCompoundWidgetAccessor* Accessor = reinterpret_cast<FCompoundWidgetAccessor*>(static_cast<SCompoundWidget*>(&PaletteRef.Get()));
	Accessor->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(MatHelperMgn->HeightRatio)
		.Padding(2.0f)
		[
			MhWidget
		]
		+ SVerticalBox::Slot()
		[
			OriginalContent //原有的控件
		]
	];

	//注册场景窗口（材质编辑器内）.
	TSharedPtr<FTabManager> TabManager = MatEditor->GetTabManager();
	if (TabManager.IsValid())
	{
		TabManager->RegisterTabSpawner(MaterialSceneViewEditorTabName, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnSceneEditorView))
			.SetDisplayName(FText::FromString(L"\u573a\u666f\u89c6\u56fe"))
			.SetGroup(TabManager->GetLocalWorkspaceMenuRoot())
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));

		// UE4.26 addition: texture browser dockable inside the material editor.
		TabManager->RegisterTabSpawner(TextureBrowserTabName, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnTextureBrowser))
			.SetDisplayName(FText::FromString(L"\u8d34\u56fe\u6d4f\u89c8\u5668"))
			.SetGroup(TabManager->GetLocalWorkspaceMenuRoot())
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));
	}
}

void FMatHelperModule::NiagaraToolBarExtend(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection(TEXT("MatHelper"));
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateStatic(&PlayNiagaraOnEditorWorld)),
			FName(TEXT("Play Niagara")),
			FText::FromString(L"\u64ad\u653e Niagara"),
			FText::FromString(L"\u64ad\u653e Niagara"),
			FSlateIcon(TEXT("SimpleButtonStyle"),"SimpleButton.Niagara"),
			EUserInterfaceActionType::Button
		);
	}
	ToolbarBuilder.EndSection();
}

void FMatHelperModule::InitNiagaraEditorHook()
{
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Compile", EExtensionHook::After, nullptr, FToolBarExtensionDelegate::CreateRaw(this, &FMatHelperModule::NiagaraToolBarExtend));
		NiagaraEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);;
	}
	// UE4.26: ILevelSequenceModule has no OnNewActorTrackAdded — Niagara auto-track (OverrideNiagaraSequenceMode) not available.

	if(MatHelperMgn->CreateNiagaraAutoPlaySelection)
	{
		RegisterNiagaraAutoPlayer();
	}
}


TSharedRef<ISceneOutlinerColumn> MatHelperSpace::OnCreateOutlinerColumn(ISceneOutliner& SceneOutliner)
{
	return MakeShareable(new FOuterlineSelectionLockCol(SceneOutliner));
}

void FMatHelperModule::PlayNiagaraOnEditorWorld()
{
	TArray<AActor*> NiagaraActors;
	UWorld* World = GEditor->GetEditorWorldContext().World();
	UGameplayStatics::GetAllActorsOfClass(World,ANiagaraActor::StaticClass(),NiagaraActors);
	int32 PlayedCount = 0;
	for(AActor* NiagaraActor : NiagaraActors)
	{
		auto Niagara = Cast<ANiagaraActor>(NiagaraActor);
		if(Niagara->Tags.Contains("NiagaraAutoPlay"))
		{
			UNiagaraComponent* NiagaraComponent = Niagara->GetNiagaraComponent();
			NiagaraComponent->Activate(true);
			NiagaraComponent->ReregisterComponent();
			PlayedCount++;
		}
	}
	if (PlayedCount == 0)
	{
		Get().EditorNotify(FString(L"\u672a\u627e\u5230\u5e26 NiagaraAutoPlay \u6807\u7b7e\u7684 Actor\uff08\u8bf7\u5148\u5728\u5927\u7eb2\u89c6\u56fe\u4e2d\u52fe\u9009\uff09"),SNotificationItem::CS_Fail);
	}
}

void FMatHelperModule::EditorNotify(const FString& NotifyInfo, SNotificationItem::ECompletionState State)
{
	FNotificationInfo Info( FText::FromString(NotifyInfo) );
	Info.FadeInDuration = 0.5f;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = 5.0f;
	const auto NotificationItem = FSlateNotificationManager::Get().AddNotification( Info );
	NotificationItem->SetCompletionState(State);
	NotificationItem->ExpireAndFadeout();
}

void FMatHelperModule::RefreshAllWidgetButton()
{
	for(auto MhWidget : MhWidgets)
	{
		if(MhWidget.Pin().IsValid())
		{
			MhWidget.Pin().Get()->InitialButton();
		}
	}
}

TSharedRef<SDockTab> FMatHelperModule::OnSpawnButtonInfoEditor(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
					SNew(SButtonInfoEditor,MatHelperMgn->NodeButtonInfo)
			];
}


TSharedRef<SDockTab> FMatHelperModule::OnSpawnSceneEditorView(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
				SNew(SceneEditorView)
		];

}

TSharedRef<SDockTab> FMatHelperModule::OnSpawnContentBrowser(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab);

	// Each tab gets its own browser instance; the tab id keeps them independent.
	// (4.26: FContentBrowserModule::Get is a const member — fetch the module instance.)
	const FString InstanceName = FString(TEXT("MatHelperContentBrowser_")) + SpawnTabArgs.GetTabId().ToString();
	NewTab->SetContent(
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().CreateContentBrowser(FName(*InstanceName), NewTab, nullptr)
	);
	return NewTab;
}

TSharedRef<SDockTab> FMatHelperModule::OnSpawnTextureBrowser(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(FText::FromString(L"\u8d34\u56fe\u6d4f\u89c8\u5668"))
		[
			SNew(SMatHelperTextureBrowser)
		];
}


void FMatHelperModule::RegisterButton()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Tools");
	{
		FToolMenuSection& MenuSection = Menu->AddSection("MatHelper", FText::FromString(L"\u6750\u8d28\u52a9\u624b"));
		MenuSection.AddDynamicEntry("MatHelper", FNewToolMenuSectionDelegate::CreateLambda([&](FToolMenuSection& InSection)
		{
#pragma region MatHelper
			InSection.AddEntry(FToolMenuEntry::InitSubMenu("MatHelper",FText::FromString("MatHelper"),FText::GetEmpty(),FNewToolMenuDelegate::CreateLambda([&](UToolMenu* InToolMenu)
			{
				FToolMenuSection& Section = InToolMenu->FindOrAddSection("MatHelper");
				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"MatHelper",
					FText::FromString(L"\u6750\u8d28\u52a9\u624b"),
					FText::FromString(L"\u6253\u5f00\u6750\u8d28\u52a9\u624b\u7ba1\u7406\u5668"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						if (GEditor)
						{
							GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset("/MatHelper/MatHelper.MatHelper");
						}
					}))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"SwitchCN",
					FText::FromString(L"\u5207\u6362\u4e2d\u82f1"),
					FText::FromString(L"\u5728\u4e2d\u82f1\u6587\u4e4b\u95f4\u5207\u6362\u8bed\u8a00"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						if(UKismetInternationalizationLibrary::GetCurrentLanguage() == "en")
						{
							UKismetInternationalizationLibrary::SetCurrentLanguage("zh-cn",true);
						}
						else
						{
							UKismetInternationalizationLibrary::SetCurrentLanguage("en",true);
						}
					}))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"SceneView",
					FText::FromString(L"\u573a\u666f\u89c6\u56fe"),
					FText::FromString(L"\u521b\u5efa\u4e00\u4e2a\u573a\u666f\u89c6\u56fe"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([&]()
					{
						FGlobalTabmanager::Get()->TryInvokeTab(SceneViewEditorTabName);
					}))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"TextureBrowser",
					FText::FromString(L"\u8d34\u56fe\u6d4f\u89c8\u5668"),
					FText::FromString(L"\u6253\u5f00\u6750\u8d28\u8d34\u56fe\u6d4f\u89c8\u5668\uff08\u53ef\u62d6\u62fd\u8d34\u56fe\u5230\u6750\u8d28\u56fe\uff09"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.ContentBrowser"),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						FGlobalTabmanager::Get()->TryInvokeTab(TextureBrowserTabName);
					}))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"Lock",
					FText::FromString(L"\u9501\u5b9a"),
					FText::FromString(L"\u9501\u5b9a\u9009\u4e2d\u8d44\u4ea7"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateRaw(this, &FMatHelperModule::ToggleAssetFlag,true))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"UnLock",
					FText::FromString(L"\u89e3\u9501"),
					FText::FromString(L"\u89e3\u9501\u9009\u4e2d\u8d44\u4ea7"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateRaw(this, &FMatHelperModule::ToggleAssetFlag,false))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"Restart",
					FText::FromString(L"\u91cd\u542f\u5f15\u64ce"),
					FText::FromString(L"\u91cd\u542f\u865a\u5e7b\u5f15\u64ce"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						bool bRestart = false;
						bRestart = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo,FText::FromString(L"\u91cd\u542f\u5f15\u64ce\uff1f"));
						if(bRestart)
						{
							FUnrealEdMisc::Get().RestartEditor(false);
						}
					}))
				));
			}),
			false,
			FSlateIcon(FEditorStyle::GetStyleSetName(), "MainFrame.SaveLayout") ));
#pragma  endregion

#pragma region  SceneView

			auto AddSceneViewEntry = [&](FToolMenuSection& Section,const FString& Name,const FName& ID)
			{
				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
				*Name,
				FText::FromString(Name),
				FText::FromString(L"\u521b\u5efa\u573a\u666f\u89c6\u56fe"),
				FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"),
				// Capture ID by value: the menu entry outlives this function's stack frame,
				// so a by-reference capture would read freed memory when the entry is clicked.
				FUIAction(FExecuteAction::CreateLambda([ID]()
				{
					FGlobalTabmanager::Get()->TryInvokeTab(ID);
				}))));
			};

			InSection.AddEntry(FToolMenuEntry::InitSubMenu("SceneView",FText::FromString(L"\u573a\u666f\u89c6\u56fe"),FText::GetEmpty(),FNewToolMenuDelegate::CreateLambda([&](UToolMenu* InToolMenu)
			{
				FToolMenuSection& Section = InToolMenu->FindOrAddSection("SceneView");
				AddSceneViewEntry(Section,"Scene View 1",SceneViewEditorTabName1);
				AddSceneViewEntry(Section,"Scene View 2",SceneViewEditorTabName2);
				AddSceneViewEntry(Section,"Scene View 3",SceneViewEditorTabName3);
				AddSceneViewEntry(Section,"Scene View 4",SceneViewEditorTabName4);
				AddSceneViewEntry(Section,"Scene View 5",SceneViewEditorTabName5);
				AddSceneViewEntry(Section,"Scene View 6",SceneViewEditorTabName6);
				AddSceneViewEntry(Section,"Scene View 7",SceneViewEditorTabName7);
				AddSceneViewEntry(Section,"Scene View 8",SceneViewEditorTabName8);
				AddSceneViewEntry(Section,"Scene View 9",SceneViewEditorTabName9);

			}),false,FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports") ));
#pragma endregion
		}));
	}
}

void FMatHelperModule::RegisterNiagaraAutoPlayer()
{

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(
					FToolMenuEntry::InitToolBarButton(
						FSimpleButtonCommands::Get().PlayNiagaraAction,
						FText::FromString(L"\u64ad\u653e Niagara"),
						FText::FromString(L"\u64ad\u653e Niagara"),
						FSlateIcon(TEXT("SimpleButtonStyle"),"SimpleButton.Niagara")
					));
				Entry.SetCommandList(PlayNiagaraCommands);
			}
		}
	}

	{
		// UE4.26: menu path uses MaterialEditorApp (not MaterialEditor).
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("AssetEditor.MaterialEditorApp.ToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(
					FToolMenuEntry::InitToolBarButton(
						FSimpleButtonCommands::Get().PlayNiagaraAction,
						FText::FromString(L"\u64ad\u653e Niagara"),
						FText::FromString(L"\u64ad\u653e Niagara"),
						FSlateIcon(TEXT("SimpleButtonStyle"),"SimpleButton.Niagara")
					));
				Entry.SetCommandList(PlayNiagaraCommands);
			}
		}
	}

	{
		// UE4.26: menu path uses MaterialInstanceEditorApp.
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("AssetEditor.MaterialInstanceEditorApp.ToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(
					FToolMenuEntry::InitToolBarButton(
						FSimpleButtonCommands::Get().PlayNiagaraAction,
						FText::FromString(L"\u64ad\u653e Niagara"),
						FText::FromString(L"\u64ad\u653e Niagara"),
						FSlateIcon(TEXT("SimpleButtonStyle"),"SimpleButton.Niagara")
					));
				Entry.SetCommandList(PlayNiagaraCommands);
			}
		}
	}

	auto& SceneOutlinerModule = FModuleManager::LoadModuleChecked<FSceneOutlinerModule>("SceneOutliner");
	SceneOutliner::FColumnInfo ColumnInfo(
		SceneOutliner::EColumnVisibility::Visible,
		1,
		FCreateSceneOutlinerColumn::CreateStatic(&::OnCreateOutlinerColumn)
		);
	SceneOutliner::FDefaultColumnInfo SceneOutlinerColumnInfo(ColumnInfo);
	SceneOutlinerModule.RegisterDefaultColumnType<FOuterlineSelectionLockCol>(SceneOutlinerColumnInfo);

}

void FMatHelperModule::ToggleAssetFlag(bool bIsLock)
{
	int32 ConvertFileNum = 0;
	TArray<FAssetData> ObjectsToExport;
	AssetSelectionUtils::GetSelectedAssets(ObjectsToExport);
	if(bIsLock)
	{
		for(auto AssetData : ObjectsToExport)
		{
			UPackage* Package = AssetData.GetAsset()->GetOutermost();
			if(Package && !Package->HasAnyPackageFlags(PKG_DisallowExport))
			{
				Package->SetPackageFlags(PKG_DisallowExport);
				ConvertFileNum = ConvertFileNum + 1;
			}
		}
	}
	else
	{
		for(auto AssetData : ObjectsToExport)
		{
			UPackage* Package = AssetData.GetAsset()->GetOutermost();
			if(Package && Package->HasAnyPackageFlags(PKG_DisallowExport))
			{
				Package->ClearPackageFlags(PKG_DisallowExport);
				ConvertFileNum = ConvertFileNum + 1;
			}
		}
	}

	FString NotifyStr = FString::Printf(TEXT("%s %d "), bIsLock ? L"\u9501\u5b9a" : L"\u89e3\u9501", ConvertFileNum);
	NotifyStr += L"\u4e2a\u6587\u4ef6";
	EditorNotify(NotifyStr, SNotificationItem::CS_Success);
}



// UE4.26: AddDefaultSystemTracks removed — ILevelSequenceModule has no OnNewActorTrackAdded delegate.
// UE4.26: SMaterialPalette::Tick removed — not a virtual member in 4.26's SGraphPalette.

// --- BEGIN ---Level Map Open Tool
static bool MatHelperSpace::HasPlayWorld()
{
	return GEditor->PlayWorld != nullptr;
}

static bool MatHelperSpace::HasNoPlayWorld()
{
	return !HasPlayWorld();
}

static bool MatHelperSpace::CanShowCommonMaps()
{
	return HasNoPlayWorld() && GetDefault<UMatHelperSettings>()->CommonEditorMaps.Num() > 0;
}

static void MatHelperSpace::OpenCommonMap_Clicked(const FString MapPath)
{
	if (ensure(MapPath.Len()))
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(MapPath);
	}
}

static TSharedRef<SWidget> MatHelperSpace::GetCommonMapsDropdown()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	for (const FSoftObjectPath& Path : GetDefault<UMatHelperSettings>()->CommonEditorMaps)
	{
		if (!Path.IsValid())
		{
			continue;
		}

		const FText DisplayName = FText::FromString(Path.GetAssetName());
		MenuBuilder.AddMenuEntry(
			DisplayName,
			FText::FromString(L"\u5728\u7f16\u8f91\u5668\u4e2d\u6253\u5f00\u6b64\u5730\u56fe"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&OpenCommonMap_Clicked, Path.ToString()),
				FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateStatic(&HasNoPlayWorld)
			)
		);
	}

	return MenuBuilder.MakeWidget();
}


static void MatHelperSpace::RegisterGameEditorMenus()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	FToolMenuSection& Section = Menu->AddSection("PlayGameExtensions", TAttribute<FText>(), FToolMenuInsert("Play", EToolMenuInsertType::After));

	FToolMenuEntry CommonMapEntry = FToolMenuEntry::InitComboButton(
	"CommonMapOptions",
	FUIAction(FExecuteAction(),FCanExecuteAction::CreateStatic(&HasNoPlayWorld),FIsActionChecked(),FIsActionButtonVisible::CreateStatic(&CanShowCommonMaps)),
	FOnGetContent::CreateStatic(&GetCommonMapsDropdown),
	FText::FromString(L"\u5e38\u7528\u5730\u56fe"),
	FText::FromString(L"\u7f16\u8f91\u5668\u4e2d\u5e38\u7528\u7684\u5730\u56fe"),
	FSlateIcon(FEditorStyle::GetStyleSetName(), "Icons.Level")
	);
	// UE4.26: FToolMenuEntry has no StyleNameOverride member.
	Section.AddEntry(CommonMapEntry);
}
// --- END ---Level Map Open Tool

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMatHelperModule, MatHelper)
/*
 *
*/
