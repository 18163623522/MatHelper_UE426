
#pragma execution_character_set("utf-8")
// Copyright AKaKLya 2024
// UE4.26 port: SceneOutliner::FActorTreeItem in namespace; ITreeItem has no CastTo.

#include "OuterlineSelectionCol.h"

#include "ActorTreeItem.h"
#include "ITreeItem.h"
#include "MatHelper.h"
#include "NiagaraActor.h"
#include "ButtonClass/SimpleButtonStyle.h"


SHeaderRow::FColumn::FArguments FOuterlineSelectionLockCol::ConstructHeaderRowColumn()
{
	const ISlateStyle* Style = &FSimpleButtonStyle::Get();
	return SHeaderRow::Column(GetColumnID())
	.FixedWidth(24.f)
	.HAlignHeader(HAlign_Center)
	.VAlignHeader(VAlign_Center)
	.HAlignCell(HAlign_Center)
	.VAlignCell(VAlign_Center)
	.DefaultTooltip(FText::FromString(L"Niagara \u9501\u5b9a"))
	[
		SNew(SImage)
		.ColorAndOpacity(FSlateColor::UseForeground())
		.Image(Style->GetBrush("SimpleButton.Niagara"))
	];

}

const TSharedRef<SWidget> FOuterlineSelectionLockCol::ConstructRowWidget(SceneOutliner::FTreeItemRef TreeItem,
	const STableRow<SceneOutliner::FTreeItemPtr>& Row)
{
	// UE4.26: ITreeItem has no CastTo. Use StaticCastSharedRef to FActorTreeItem.
	TSharedRef<SceneOutliner::FActorTreeItem> ActorTreeItem = StaticCastSharedRef<SceneOutliner::FActorTreeItem>(TreeItem);
	if (!ActorTreeItem->Actor.IsValid() || !Cast<ANiagaraActor>(ActorTreeItem->Actor.Get()))
	{
		return SNullWidget::NullWidget;
	}

	const bool bIsActorLocked = ActorTreeItem->Actor.Get()->Tags.Contains("NiagaraAutoPlay");

	return SNew(SCheckBox)
	.Visibility(EVisibility::Visible)
	.HAlign(HAlign_Center)
	.IsChecked(bIsActorLocked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
	.OnCheckStateChanged(this, &FOuterlineSelectionLockCol::OnCheckStateChanged, ActorTreeItem->Actor);
}

void FOuterlineSelectionLockCol::OnCheckStateChanged(ECheckBoxState NewState, TWeakObjectPtr<AActor> Actor)
{
	FMatHelperModule& MatHelper = FMatHelperModule::Get();

	switch (NewState)
	{
	case ECheckBoxState::Unchecked:
		Actor.Get()->Tags.Remove("NiagaraAutoPlay");
		break;

	case ECheckBoxState::Checked:
		Actor.Get()->Tags.Add("NiagaraAutoPlay");
		break;

	case ECheckBoxState::Undetermined:
		break;

	default:
		break;
	}
}
