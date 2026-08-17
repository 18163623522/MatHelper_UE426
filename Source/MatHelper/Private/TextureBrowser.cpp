// Copyright AKaKLya 2024
// UE4.26 addition: material-focused texture browser with folder / name modes.
// The folder tree defaults to texture-bearing folders only (project-wide list
// comes from the asset registry; empty folders are hidden).

#include "TextureBrowser.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCube.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "MatHelperTextureBrowser"

void SMatHelperTextureBrowser::Construct(const FArguments& InArgs)
{
	bFolderMode = true;
	bShowAllFolders = false;
	CurrentPath = TEXT("/Game");

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.SearchAllAssets(true);

	// Precise filter: only folders that actually hold texture assets (plus their
	// ancestor folders so the tree stays connected). GetAllCachedPaths returns
	// every folder with ANY asset — that was why so many "empty" (texture-less)
	// folders showed up before.
	{
		TArray<FAssetData> TextureAssets;
		FARFilter Filter;
		Filter.ClassNames.Add(UTexture2D::StaticClass()->GetFName());
		Filter.ClassNames.Add(UTextureCube::StaticClass()->GetFName());
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Add(FName(TEXT("/Game")));
		AssetRegistry.GetAssets(Filter, TextureAssets);

		TSet<FString> TextureFoldersSet;
		for (const FAssetData& Asset : TextureAssets)
		{
			FString Path = Asset.PackagePath.ToString();
			// Add this folder and every ancestor up to /Game.
			while (Path.Len() > 5) // 5 == len("/Game")
			{
				if (TextureFoldersSet.Contains(Path)) { break; }
				TextureFoldersSet.Add(Path);
				Path = FPaths::GetPath(Path);
			}
			TextureFoldersSet.Add(TEXT("/Game"));
		}
		AllTextureFolders = TextureFoldersSet.Array();
		AllTextureFolders.Sort();
	}

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 8, 0)
			[
				SNew(SCheckBox)
				.IsChecked(this, &SMatHelperTextureBrowser::IsFolderMode_ReturnState)
				.OnCheckStateChanged(this, &SMatHelperTextureBrowser::OnFolderModeChecked)
				.Content()
				[
					SNew(STextBlock).Text(FText::FromString(L"\u6309\u6587\u4ef6\u5939"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 8, 0)
			[
				SNew(SCheckBox)
				.IsChecked(this, &SMatHelperTextureBrowser::IsNameMode_ReturnState)
				.OnCheckStateChanged(this, &SMatHelperTextureBrowser::OnNameModeChecked)
				.Content()
				[
					SNew(STextBlock).Text(FText::FromString(L"\u6309\u540d\u79f0"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 8, 0)
			[
				SNew(SBox)
				.Visibility(TAttribute<EVisibility>(this, &SMatHelperTextureBrowser::GetShowAllFoldersRowVisibility))
				[
					SNew(SCheckBox)
					.IsChecked(bShowAllFolders ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.OnCheckStateChanged(this, &SMatHelperTextureBrowser::OnShowAllFoldersChecked)
					.Content()
					[
						SNew(STextBlock).Text(FText::FromString(L"\u663e\u793a\u7a7a\u6587\u4ef6\u5939"))
					]
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSearchBox)
				.HintText(FText::FromString(L"\u641c\u7d22\u8d34\u56fe\u540d\u79f0"))
				.OnTextChanged(this, &SMatHelperTextureBrowser::OnSearchTextChanged)
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2.0f)
		[
			SNew(SSplitter)

			+ SSplitter::Slot()
			.Value(0.3f)
			[
				SAssignNew(TreeContainer, SBox)
				.Visibility(TAttribute<EVisibility>(this, &SMatHelperTextureBrowser::GetFolderTreeVisibility))
			]

			+ SSplitter::Slot()
			.Value(0.7f)
			[
				SAssignNew(PickerContainer, SBox)
			]
		]
	];

	RebuildFolderTree();
	RebuildAssetPicker();
}

ECheckBoxState SMatHelperTextureBrowser::IsFolderMode_ReturnState() const
{
	return bFolderMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SMatHelperTextureBrowser::IsNameMode_ReturnState() const
{
	return bFolderMode ? ECheckBoxState::Unchecked : ECheckBoxState::Checked;
}

void SMatHelperTextureBrowser::OnFolderModeChecked(ECheckBoxState NewState)
{
	if (NewState == ECheckBoxState::Checked && !bFolderMode)
	{
		bFolderMode = true;
		RebuildAssetPicker();
	}
}

void SMatHelperTextureBrowser::OnNameModeChecked(ECheckBoxState NewState)
{
	if (NewState == ECheckBoxState::Checked && bFolderMode)
	{
		bFolderMode = false;
		RebuildAssetPicker();
	}
}

EVisibility SMatHelperTextureBrowser::GetFolderTreeVisibility() const
{
	return bFolderMode ? EVisibility::All : EVisibility::Collapsed;
}

EVisibility SMatHelperTextureBrowser::GetShowAllFoldersRowVisibility() const
{
	return bFolderMode ? EVisibility::Visible : EVisibility::Collapsed;
}

void SMatHelperTextureBrowser::OnShowAllFoldersChecked(ECheckBoxState NewState)
{
	bShowAllFolders = (NewState == ECheckBoxState::Checked);
	RebuildFolderTree();
}

void SMatHelperTextureBrowser::CollectFolders(const FString& RootPath, TArray<FTextureFolderItemPtr>& OutFolders) const
{
	for (const FString& Folder : AllTextureFolders)
	{
		// Strict children only (never the root itself).
		if (Folder.Len() > RootPath.Len() && Folder.StartsWith(RootPath + TEXT("/")))
		{
			OutFolders.Add(MakeShareable(new FString(Folder)));
		}
	}
	OutFolders.Sort([](const FTextureFolderItemPtr& A, const FTextureFolderItemPtr& B) { return *A < *B; });
}

void SMatHelperTextureBrowser::RebuildFolderTree()
{
	DisplayedFolders.Reset();
	for (const FString& Folder : AllTextureFolders)
	{
		DisplayedFolders.Add(MakeShareable(new FString(Folder)));
	}

	if (FolderTree.IsValid())
	{
		FolderTree->RequestTreeRefresh();
	}
	else if (TreeContainer.IsValid())
	{
		TreeContainer->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(FolderTree, STreeView<FTextureFolderItemPtr>)
				.TreeItemsSource(&DisplayedFolders)
				.OnGenerateRow(this, &SMatHelperTextureBrowser::GenerateFolderRow)
				.OnGetChildren(this, &SMatHelperTextureBrowser::GetFolderChildren)
				.OnSelectionChanged(this, &SMatHelperTextureBrowser::OnFolderSelectionChanged)
				.OnExpansionChanged(this, &SMatHelperTextureBrowser::OnFolderExpansionChanged)
				.SelectionMode(ESelectionMode::Single)
			]
		);
	}
}

TSharedRef<ITableRow> SMatHelperTextureBrowser::GenerateFolderRow(FTextureFolderItemPtr InFolder, const TSharedRef<STableViewBase>& OwnerTable)
{
	const FString FolderName = FPaths::GetCleanFilename(*InFolder);
	return SNew(STableRow<FTextureFolderItemPtr>, OwnerTable)
		[
			SNew(STextBlock).Text(FText::FromString(FolderName))
		];
}

void SMatHelperTextureBrowser::GetFolderChildren(FTextureFolderItemPtr InFolder, TArray<FTextureFolderItemPtr>& OutChildren)
{
	CollectFolders(*InFolder, OutChildren);
}

void SMatHelperTextureBrowser::OnFolderExpansionChanged(FTextureFolderItemPtr InFolder, bool bShouldBeExpanded)
{
	if (bShouldBeExpanded)
	{
		ExpandedFolders.Add(*InFolder);
	}
	else
	{
		ExpandedFolders.Remove(*InFolder);
	}
}

void SMatHelperTextureBrowser::OnFolderSelectionChanged(FTextureFolderItemPtr InFolder, ESelectInfo::Type SelectInfo)
{
	if (InFolder.IsValid())
	{
		CurrentPath = *InFolder;
		if (bFolderMode)
		{
			RebuildAssetPicker();
		}
	}
}

void SMatHelperTextureBrowser::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText.ToString();
	if (!bFolderMode)
	{
		RebuildAssetPicker();
	}
}

bool SMatHelperTextureBrowser::ShouldFilterAsset(const FAssetData& AssetData) const
{
	if (SearchText.Len() > 0 && !AssetData.AssetName.ToString().Contains(SearchText, ESearchCase::IgnoreCase))
	{
		return true;
	}
	return false;
}

void SMatHelperTextureBrowser::RebuildAssetPicker()
{
	if (!PickerContainer.IsValid())
	{
		return;
	}

	FAssetPickerConfig Config;
	Config.Filter.ClassNames.Add(UTexture2D::StaticClass()->GetFName());
	Config.Filter.ClassNames.Add(UTextureCube::StaticClass()->GetFName());
	// UE4.26: PackagePaths is TArray<FName> (FString only in UE5); recursive so a
	// selected folder also shows its subfolders' textures.
	Config.Filter.bRecursivePaths = true;
	Config.Filter.PackagePaths.Add(FName(*(bFolderMode ? CurrentPath : FString(TEXT("/Game")))));
	Config.InitialAssetViewType = EAssetViewType::Tile;
	Config.SaveSettingsName = TEXT("MatHelper.TextureBrowser");
	// Dragging a texture into the material graph creates a TextureSample natively.
	Config.bAllowDragging = true;
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([](const FAssetData& AssetData)
	{
		if (GEditor)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetData.GetAsset());
		}
	});
	Config.OnShouldFilterAsset = FOnShouldFilterAsset::CreateRaw(this, &SMatHelperTextureBrowser::ShouldFilterAsset);

	PickerContainer->SetContent(
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().CreateAssetPicker(Config)
	);
}

#undef LOCTEXT_NAMESPACE
