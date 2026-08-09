// Copyright AKaKLya 2024


#include "SceneEditorView.h"

#include "LevelEditorViewport.h"
#include "MatHelper.h"
#include "MatHelperMgn.h"
#include "SceneEditorViewToolBar.h"
#include "Engine/Selection.h"

void SceneEditorView::Construct(const FArguments& InArgs)
{
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SceneEditorView::SceneEditorView()
{

}

SceneEditorView::~SceneEditorView()
{

}

void SceneEditorView::UpdateViewPortLocation()
{
	ESceneViewMethod bSelectCamera = FMatHelperModule::Get().MatHelperMgn->SceneViewMethod;
	switch (bSelectCamera)
	{
	case ESceneViewMethod::Auto:
		{
			TArray<FLevelEditorViewportClient*> EditorViewPort = GEditor->GetLevelViewportClients();
			if (EditorViewPort.Num() > 1)
			{
				Client.Get()->SetViewLocation(EditorViewPort[1]->GetViewLocation());
				Client.Get()->SetViewRotation(EditorViewPort[1]->GetViewRotation());
			}
			break;
		}

	case ESceneViewMethod::SelectActor:
		{
			// UE4.26: no UEditorActorSubsystem. Use GEditor->GetSelectedActors() + USelection.
			TArray<AActor*> SelectedActors;
			GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);
			if (SelectedActors.Num() > 0)
			{
				Client.Get()->SetViewLocation(SelectedActors[0]->GetActorLocation());
				Client.Get()->SetViewRotation(SelectedActors[0]->GetActorRotation());
			}
			break;
		}
	}
}

TSharedRef<FEditorViewportClient> SceneEditorView::MakeEditorViewportClient()
{
	Client = MakeShareable(new FEditorViewportClient(nullptr));
	Client.Get()->SetShowGrid();
	Client.Get()->EngineShowFlags.SetEditor(false);
	Client.Get()->SetRealtime(true);
	
	UpdateViewPortLocation();
	return Client.ToSharedRef();
}

TSharedPtr<SWidget> SceneEditorView::MakeViewportToolbar()
{
	return SNew(SceneEditorViewToolBar,this,SharedThis(this));
}

TSharedRef<SEditorViewport> SceneEditorView::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> SceneEditorView::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
	return Result;
}

void SceneEditorView::OnFloatingButtonClicked()
{
	
}

