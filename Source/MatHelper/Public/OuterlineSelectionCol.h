// Copyright AKaKLya 2024

#pragma once

#include "ISceneOutlinerColumn.h"


class MATHELPER_API FOuterlineSelectionLockCol : public ISceneOutlinerColumn
{
public:
	FOuterlineSelectionLockCol(ISceneOutliner& SceneOutliner){};
	
	virtual FName GetColumnID() override {return FName("SelectionLock");};
	
	static FName GetID() {return FName("SelectionLock");};
	
	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;
	virtual const TSharedRef< SWidget > ConstructRowWidget(SceneOutliner::FTreeItemRef TreeItem, const STableRow<SceneOutliner::FTreeItemPtr>& Row) override;

private:
	void OnCheckStateChanged(ECheckBoxState NewState,TWeakObjectPtr<AActor> Actor);

};
