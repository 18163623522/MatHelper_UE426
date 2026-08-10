
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
	// Create a transient default instance so the plugin still functions.
	if (!MatHelperMgn)
	{
		UE_LOG(LogTemp, Warning, TEXT("MatHelper: Failed to load /MatHelper/MatHelper.MatHelper — creating transient default. Please rebuild the DataAsset in 4.26."));
		MatHelperMgn = NewObject<UMatHelperMgn>(GetTransientPackage(), NAME_None, RF_Transient);
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


void FMatHelperModule::RegisterTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ButtonInfoEditorTabName, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnButtonInfoEditor))
		.SetDisplayName(FText::FromString(UTF8_TO_TCHAR("节点编辑器")))
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
			IMaterialEditor* IMatEditor = InMatEditor.Pin().Get();
			FMaterialEditor* MatEditor = static_cast<FMaterialEditor*>(IMatEditor);
			// UE4.26: direct access via #define hack (Palette is private, ChildSlot is protected).
			TSharedPtr<SMaterialPalette>& Palette = MatEditor->Palette;

			IMatEditor->OnRegisterTabSpawners().AddLambda([&](const TSharedRef<class FTabManager>& TabManager)
			{
				// 在原有控件的基础上 添加自定义的控件.
				auto MhWidget = SNew(SMatHelperWidget, MatEditor);
				MhWidgets.RemoveAll([](auto& WeakWidget){ return !WeakWidget.IsValid(); });
				MhWidgets.Add(MhWidget);

				// UE4.26: SCompoundWidget::ChildSlot is protected. Use GetChildren() to read
				// original content, then use a derived-class helper to access ChildSlot for writing.
				TSharedRef<SMaterialPalette> PaletteRef = Palette.ToSharedRef();
				FChildren* Children = PaletteRef->GetChildren();
				TSharedRef<SWidget> OriginalContent = Children->GetChildAt(0);

				// Access ChildSlot via file-scope FCompoundWidgetAccessor (reinterpret through SCompoundWidget*).
				// UE4.26: TSharedRef::Get() returns ObjectType& (reference), need & to get pointer.
				SCompoundWidget* AsCompoundWidget = static_cast<SCompoundWidget*>(&PaletteRef.Get());
				FCompoundWidgetAccessor* Accessor = reinterpret_cast<FCompoundWidgetAccessor*>(AsCompoundWidget);				Accessor->ChildSlot
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

				//注册场景窗口.
				TabManager->RegisterTabSpawner(MaterialSceneViewEditorTabName, FOnSpawnTab::CreateRaw(this, &FMatHelperModule::OnSpawnSceneEditorView))
					.SetDisplayName(FText::FromString(UTF8_TO_TCHAR("场景视图")))
					.SetGroup(TabManager->GetLocalWorkspaceMenuRoot())
					.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));
			});
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
					.SetDisplayName(FText::FromString(UTF8_TO_TCHAR("场景视图")))
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
					FText::FromString(UTF8_TO_TCHAR("场景视图")),
					FText::FromString(UTF8_TO_TCHAR("场景视图")),
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

void FMatHelperModule::NiagaraToolBarExtend(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection(TEXT("MatHelper"));
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateStatic(&PlayNiagaraOnEditorWorld)),
			FName(TEXT("Play Niagara")),
			FText::FromString(UTF8_TO_TCHAR("播放 Niagara")),
			FText::FromString(UTF8_TO_TCHAR("播放 Niagara")),
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
	for(AActor* NiagaraActor : NiagaraActors)
	{
		auto Niagara = Cast<ANiagaraActor>(NiagaraActor);
		if(Niagara->Tags.Contains("NiagaraAutoPlay"))
		{
			UNiagaraComponent* NiagaraComponent = Niagara->GetNiagaraComponent();
			NiagaraComponent->Activate(true);
			NiagaraComponent->ReregisterComponent();
		}
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


void FMatHelperModule::RegisterButton()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Tools");
	{
		FToolMenuSection& MenuSection = Menu->AddSection("MatHelper", FText::FromString(UTF8_TO_TCHAR("材质助手")));
		MenuSection.AddDynamicEntry("MatHelper", FNewToolMenuSectionDelegate::CreateLambda([&](FToolMenuSection& InSection)
		{
#pragma region MatHelper
			InSection.AddEntry(FToolMenuEntry::InitSubMenu("MatHelper",FText::FromString("MatHelper"),FText::GetEmpty(),FNewToolMenuDelegate::CreateLambda([&](UToolMenu* InToolMenu)
			{
				FToolMenuSection& Section = InToolMenu->FindOrAddSection("MatHelper");
				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"MatHelper",
					FText::FromString(UTF8_TO_TCHAR("材质助手")),
					FText::FromString(UTF8_TO_TCHAR("打开材质助手管理器")),
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
					FText::FromString(UTF8_TO_TCHAR("切换中英")),
					FText::FromString(UTF8_TO_TCHAR("在中英文之间切换语言")),
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
					FText::FromString(UTF8_TO_TCHAR("场景视图")),
					FText::FromString(UTF8_TO_TCHAR("创建一个场景视图")),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([&]()
					{
						FGlobalTabmanager::Get()->TryInvokeTab(SceneViewEditorTabName);
					}))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"Lock",
					FText::FromString(UTF8_TO_TCHAR("锁定")),
					FText::FromString(UTF8_TO_TCHAR("锁定选中资产")),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateRaw(this, &FMatHelperModule::ToggleAssetFlag,true))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"UnLock",
					FText::FromString(UTF8_TO_TCHAR("解锁")),
					FText::FromString(UTF8_TO_TCHAR("解锁选中资产")),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateRaw(this, &FMatHelperModule::ToggleAssetFlag,false))
				));

				Section.AddEntry(FToolMenuEntry::InitMenuEntry(
					"Restart",
					FText::FromString(UTF8_TO_TCHAR("重启引擎")),
					FText::FromString(UTF8_TO_TCHAR("重启虚幻引擎")),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([]()
					{
						bool bRestart = false;
						bRestart = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo,FText::FromString(UTF8_TO_TCHAR("重启引擎？")));
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
				FText::FromString(UTF8_TO_TCHAR("创建场景视图")),
				FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"),
				FUIAction(FExecuteAction::CreateLambda([&]()
				{
					FGlobalTabmanager::Get()->TryInvokeTab(ID);
				}))));
			};

			InSection.AddEntry(FToolMenuEntry::InitSubMenu("SceneView",FText::FromString(UTF8_TO_TCHAR("场景视图")),FText::GetEmpty(),FNewToolMenuDelegate::CreateLambda([&](UToolMenu* InToolMenu)
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
						FText::FromString(UTF8_TO_TCHAR("播放 Niagara")),
						FText::FromString(UTF8_TO_TCHAR("播放 Niagara")),
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
						FText::FromString(UTF8_TO_TCHAR("播放 Niagara")),
						FText::FromString(UTF8_TO_TCHAR("播放 Niagara")),
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
						FText::FromString(UTF8_TO_TCHAR("播放 Niagara")),
						FText::FromString(UTF8_TO_TCHAR("播放 Niagara")),
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

	FString NotifyStr = FString::Printf(TEXT("%s %d "), bIsLock ? UTF8_TO_TCHAR("锁定") : UTF8_TO_TCHAR("解锁"), ConvertFileNum);
	NotifyStr += UTF8_TO_TCHAR("个文件");
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
			FText::FromString(UTF8_TO_TCHAR("在编辑器中打开此地图")),
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
	FText::FromString(UTF8_TO_TCHAR("常用地图")),
	FText::FromString(UTF8_TO_TCHAR("编辑器中常用的地图")),
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
