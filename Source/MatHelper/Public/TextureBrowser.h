// Copyright AKaKLya 2024
// UE4.26 addition: material-focused texture browser with two modes:
// folder tree (texture-bearing folders only by default) and name search.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

typedef TSharedPtr<FString> FTextureFolderItemPtr;

class SMatHelperTextureBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMatHelperTextureBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// --- mode toggle -------------------------------------------------------
	bool bFolderMode;
	ECheckBoxState IsFolderMode_ReturnState() const;
	ECheckBoxState IsNameMode_ReturnState() const;
	void OnFolderModeChecked(ECheckBoxState NewState);
	void OnNameModeChecked(ECheckBoxState NewState);
	EVisibility GetFolderTreeVisibility() const;

	// --- folder tree (texture folders only by default) ----------------------
	bool bShowAllFolders;
	void OnShowAllFoldersChecked(ECheckBoxState NewState);
	EVisibility GetShowAllFoldersRowVisibility() const;
	void RebuildFolderTree();
	void CollectFolders(const FString& RootPath, TArray<FTextureFolderItemPtr>& OutFolders) const;
	TSharedRef<ITableRow> GenerateFolderRow(FTextureFolderItemPtr InFolder, const TSharedRef<STableViewBase>& OwnerTable);
	void OnFolderExpansionChanged(FTextureFolderItemPtr InFolder, bool bShouldBeExpanded);
	void OnFolderSelectionChanged(FTextureFolderItemPtr InFolder, ESelectInfo::Type SelectInfo);
	void GetFolderChildren(FTextureFolderItemPtr InFolder, TArray<FTextureFolderItemPtr>& OutChildren);

	// --- name search ---------------------------------------------------------
	void OnSearchTextChanged(const FText& NewText);

	// --- asset picker --------------------------------------------------------
	bool ShouldFilterAsset(const FAssetData& AssetData) const;
	void RebuildAssetPicker();

	FString CurrentPath;
	FString SearchText;

	TArray<FString> AllTextureFolders;                  // every folder under /Game (from asset registry)
	TArray<FTextureFolderItemPtr> DisplayedFolders;     // subset actually shown (respects bShowAllFolders)
	TSet<FString> ExpandedFolders;

	TSharedPtr<SBox> PickerContainer;
	TSharedPtr<SBox> TreeContainer;
	TSharedPtr<STreeView<FTextureFolderItemPtr>> FolderTree;
};
