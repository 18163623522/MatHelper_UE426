// Copyright AKaKLya 2024

#include "MatHelperWidget.h"
#include "EditorWidgetsModule.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "MaterialGraphNode_Knot.h"
#include "MaterialPropertyHelpers.h"
#include "MatHelper.h"
#include "MatHelperMgn.h"

// UE4.26: use #define hack to access FMaterialEditor::GraphEditor (private).
#define private public
#define protected public
#include "Editor/MaterialEditor/Private/MaterialEditor.h"
#undef private
#undef protected

#include "Kismet/KismetMathLibrary.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Comment.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Windows/WindowsPlatformApplicationMisc.h"
#include "Misc/PackageName.h"
#include "PackageTools.h"



#define LOCTEXT_NAMESPACE "MaterialPalette"

// UE4.26: direct access to FMaterialEditor::GraphEditor via #define hack — no template magic needed.

void SMatHelperWidget::Construct(const FArguments& InArgs,FMaterialEditor* InMatEditor)
{
	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
	const TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_Vertical);
	
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
	
	MatEditorInterface = InMatEditor;
	Material = Cast<UMaterial>(MatEditorInterface->GetMaterialInterface()->GetMaterial());
	
	PluginConfigPath = MatHelper.GetPluginPath().Append("/Config/");
	
	SAssignNew(GroupText,SEditableTextBox);
	SAssignNew(InstanceText,SEditableTextBox);
	SAssignNew(NodeButtonScrollBox,SScrollBox);

	RefreshMaskPinSelection();

	this->ChildSlot
	[
		NodeButtonScrollBox.ToSharedRef()
	];
	
	NodeButtonScrollBox->AddSlot()
	.Padding(5.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("MatHelper 管理器")))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
			.OnClicked_Lambda([]()
			{
				if (GEditor)
				{
					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset("/MatHelper/MatHelper.MatHelper");
				}
				return FReply::Handled();
			})
	];

	
	NodeButtonScrollBox->AddSlot()
	.Padding(5.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("场景视图")))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Lambda([&]()
		{
			MatEditorInterface->GetTabManager()->TryInvokeTab(FMatHelperModule::MaterialSceneViewEditorTabName);
			return FReply::Handled();
		})
	];
	
	NodeButtonScrollBox->AddSlot()
	.Padding(5.0f)
	[
		GroupText.ToSharedRef()	
	];
	
	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("设置分组")))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::SetNodeGroup,false,false)
	];

	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("自动分组")))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::SetNodeGroup,true,false)
	];
	

	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("全部自动分组")))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::SetNodeGroup,true,true)
	];
	
	if(MaskPinOptions.Num() > 0 )
	{
		NodeButtonScrollBox->AddSlot()
		.Padding(5.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&MaskPinOptions)
			.OnGenerateWidget_Lambda([](const TSharedPtr<FString>& InString)
			{
				return SNew(STextBlock)
				.Text(FText::FromString(*InString));
			})
			.OnSelectionChanged_Lambda([&](const TSharedPtr<FString>& NewOption,ESelectInfo::Type SelectInfo)
			{
				CurrentSelect = MaskPinOptions.Find(NewOption);
			})
			
			[
				SNew(STextBlock)
				.Text_Lambda([&]()
				{
					return FText::FromString(*MaskPinOptions[CurrentSelect]);
				})
			]
		];
	
	
		NodeButtonScrollBox->AddSlot()
		.Padding(3.0f)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("添加 Mask 引脚")))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked_Raw(this,&SMatHelperWidget::AddNodeMaskPin)
		];
	}
	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("显示引脚名称")))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Lambda([&]()
		{
			MatEditorInterface->FocusWindow();
			TArray<UObject*> SelectedNodes = MatEditorInterface->GetSelectedNodes().Array();
			if(SelectedNodes.Num() == 0)
			{
				return FReply::Handled();
			}
			
			if(CheckNode(SelectedNodes[0]) == false)
			{
				return FReply::Handled();
			}

			UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(SelectedNodes[0]);
			MatNode->MaterialExpression->bShowOutputNameOnPin = !MatNode->MaterialExpression->bShowOutputNameOnPin;
			MatNode->RecreateAndLinkNode();
			MatEditorInterface->UpdateMaterialAfterGraphChange();
			return FReply::Handled();
		})
	];


	
	NodeButtonScrollBox->AddSlot()
	.Padding(5.0f)
	[
		InstanceText.ToSharedRef()
	];
	
	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("创建实例")))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::CreateInstance)
	];
	
	NodeButtonScrollBox->AddSlot()
    	.Padding(3.0f)
    	[
    		SNew(SButton)
    		.Text(FText::FromString(TEXT("折射")))
    		.VAlign(VAlign_Center)
    		.HAlign(HAlign_Center)
    		.OnClicked_Raw(this,&SMatHelperWidget::ToggleRefraction)
    	];

	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("修复函数节点")))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::FixFunctionNode)
	];
	
	NodeButtonScrollBox->AddSlot()
	.Padding(3.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("自动命名")))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked_Raw(this,&SMatHelperWidget::RemoveParameterType)
	];
	
	InitialButton();
}

FReply SMatHelperWidget::SetNodeGroup(bool AutoGroup,bool AllGroup) const
{
	bool ShouldRefresh = false;

	if(AllGroup == true)
	{
		const auto GraphEdPtr =  MatEditorInterface->GraphEditor;
		if(const auto GraphEd = GraphEdPtr.Get())
		{
			GraphEd->SelectAllNodes();
		}
	}
	const FMatHelperModule& MatHelper = FMatHelperModule::Get();
	TArray<FString> Names = MatHelper.MatHelperMgn->AutoGroupKeys;
	
	auto SelectedNodes = MatEditorInterface->GetSelectedNodes();
	MatEditorInterface->FocusWindow();
	
	FString GroupName = GroupText->GetText().ToString(); // 提前获取组名

	const auto ProcessGroup = [&](auto* Parameter)
	{
		if (AutoGroup)
		{
			for (const FString& Name : Names)
			{
				if (Parameter->ParameterName.ToString().Contains(Name))
				{
					Parameter->Group = *Name;
					break; // 找到第一个匹配项后退出
				}
			}
		}
		else
		{
			Parameter->Group = *GroupName;
		}
		ShouldRefresh = true;
	};
	
	for(UObject* Node : SelectedNodes)
	{
		if(UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node))
		{
			if(CheckNode(MatNode) == false) {continue;}

			if(UMaterialExpressionParameter* Parameter = Cast<UMaterialExpressionParameter>(MatNode->MaterialExpression))
			{
				ProcessGroup(Parameter);
			}
			else if(UMaterialExpressionTextureSampleParameter* TexParameter = Cast<UMaterialExpressionTextureSampleParameter>(MatNode->MaterialExpression))
			{
				ProcessGroup(TexParameter);
			}
		}
	}
	
	if(ShouldRefresh)
	{
		MatEditorInterface->UpdateMaterialAfterGraphChange();
	}
	return FReply::Handled();
}

FReply SMatHelperWidget::AddNodeMaskPin()
{
	MatEditorInterface->FocusWindow();
	TArray<UObject*> SelectedNodes = MatEditorInterface->GetSelectedNodes().Array();
	if(SelectedNodes.Num() == 0)
	{
		return FReply::Handled();
	}
	
	if(CheckNode(SelectedNodes[0]) == false)
	{
		return FReply::Handled();
	}

	UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(SelectedNodes[0]);

	UMaterialExpression* Expression = MatNode->MaterialExpression;
	TArray<FExpressionOutput>& Outputs = Expression->Outputs;

	bool Found = false;
	int Index = -1;
	for(const auto& Output : Outputs)
	{
		FLinearColor PinMask = FLinearColor(Output.MaskR,Output.MaskG,Output.MaskB,Output.MaskA);
		Index = Index + 1;
		if(PinMask == MaskPinInfo[CurrentSelect])
		{
			Found = true;
			break;
		}
	}

	if(!Found)
	{
		Outputs.Add(FExpressionOutput(FName(**MaskPinOptions[CurrentSelect].Get()), 1,
			(int32)MaskPinInfo[CurrentSelect].R, (int32)MaskPinInfo[CurrentSelect].G,
			(int32)MaskPinInfo[CurrentSelect].B, (int32)MaskPinInfo[CurrentSelect].A));
	}
	else
	{
		Outputs.RemoveAt(Index);
	}
	
	MatEditorInterface->FocusWindow();
	MatNode->RecreateAndLinkNode();
	MatEditorInterface->UpdateMaterialAfterGraphChange();
	
	return FReply::Handled();
}



FReply SMatHelperWidget::CreateInstance()
{
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
	FString TargetPath = Material->GetPathName();
	const FString BaseName = Material->GetName();
	TargetPath.ReplaceInline(*BaseName,*FString(""));
	TargetPath.ReplaceInline(*FString("."),*FString(""));

	FString NewBaseName = BaseName;
	if(BaseName.Left(2) == "M_")
	{
		NewBaseName.ReplaceInline(*FString("M_"),*FString("MI_"));
	}
	else
	{
		NewBaseName = "MI_" + BaseName;
	}
	FString InText = InstanceText->GetText().ToString();

	if(InText == "")
	{
		InText = "Inst" + FString::FromInt(UKismetMathLibrary::RandomIntegerInRange(0,99));
	}
	
	const FString NewName = NewBaseName + "_" + InText;


	const FString NewPath = TargetPath + NewName;

	// UE4.26: no UEditorAssetSubsystem. Use LoadObject to check existence + DuplicateObject + SavePackage.
	if (LoadObject<UObject>(nullptr, *NewPath) != nullptr)
	{
		MatHelper.EditorNotify(TEXT("创建失败 - 此实例已存在"),SNotificationItem::CS_Fail);
		return FReply::Handled();
	}

	// Load the empty MI template and duplicate it.
	UMaterialInstance* EmptyMI = LoadObject<UMaterialInstance>(nullptr, *MIEmptyPath);
	if (!EmptyMI)
	{
		// UE4.26: MI_Empty.uasset may be UE5 format. Create a new MIC directly.
		UE_LOG(LogTemp, Warning, TEXT("MatHelper: Failed to load MI_Empty template — creating new MIC directly."));
		UPackage* NewPackage = CreatePackage(nullptr, *TargetPath);
		UMaterialInstanceConstant* NewMIC = NewObject<UMaterialInstanceConstant>(NewPackage, *NewName, RF_Public | RF_Standalone);
		NewMIC->Parent = Material;
		NewMIC->PreEditChange(nullptr);
		NewMIC->PostEditChange();
		NewPackage->SetDirtyFlag(true);
		UPackage::SavePackage(NewPackage, NewMIC, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *NewPath);

		TArray<UObject*> AssetList;
		AssetList.Add(NewMIC);
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetList);
		if (GEditor)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(NewMIC);
		}
		return FReply::Handled();
	}
	UPackage* NewPackage = CreatePackage(nullptr, *TargetPath);
	UMaterialInstance* NewMi = DuplicateObject(EmptyMI, NewPackage, *NewName);
	NewMi->Parent = Material;

	// Save the package so it appears in the content browser.
	NewMi->PreEditChange(nullptr);
	NewMi->PostEditChange();
	NewPackage->SetDirtyFlag(true);
	UPackage::SavePackage(NewPackage, NewMi, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *NewPath);
	
	UMaterialInstanceConstant* ConstMat = static_cast<UMaterialInstanceConstant*>(NewMi);
	const auto MaterialEditorInstance = NewObject<UMaterialEditorInstanceConstant>(GetTransientPackage(), NAME_None, RF_Transactional);
	MaterialEditorInstance->SetSourceInstance(ConstMat);

	const int32 GroupNum = MaterialEditorInstance->ParameterGroups.Num();
	for (int32 GroupIdx = 0; GroupIdx <GroupNum ; ++GroupIdx)
	{
		FEditorParameterGroup& ParameterGroup = MaterialEditorInstance->ParameterGroups[GroupIdx];
		
		int32 ParameterNum = ParameterGroup.Parameters.Num();
		for (int32 ParamIdx = 0; ParamIdx < ParameterNum; ++ParamIdx)
		{
			UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
			FMaterialPropertyHelpers::OnOverrideParameter(true,Parameter,MaterialEditorInstance);
		}
	}
	
	TArray<UObject*> AssetList;
	AssetList.Add(NewMi);
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(AssetList);
	if (GEditor)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(NewMi);
	}
	
	return FReply::Handled();
}

FReply SMatHelperWidget::ToggleRefraction() const
{
	// UE4.26: RefractionMethod → RefractionMode; RM_None not available, toggle between two modes.
	auto& Ref = MatEditorInterface->GetMaterialInterface()->GetMaterial()->RefractionMode;
	if (Ref == RM_IndexOfRefraction)
	{
		Ref = RM_PixelNormalOffset;
	}
	else
	{
		Ref = RM_IndexOfRefraction;
	}
	const auto BaseRootNode =  MatEditorInterface->GetMaterialInterface()->GetMaterial()->MaterialGraph->RootNode;
	Cast<UMaterialGraphNode_Root>(BaseRootNode)->ReconstructNode();
	MatEditorInterface->UpdateMaterialAfterGraphChange();
	return FReply::Handled();
}

FReply SMatHelperWidget::FixFunctionNode() const
{
	bool bNeedRefreshNode = false;
	auto Nodes = MatEditorInterface->GetSelectedNodes().Array();
	for(const auto Node : Nodes)
	{
		UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node);
		if(Cast<UMaterialExpressionMaterialFunctionCall>(MatNode->MaterialExpression))
		{
			MatNode->RecreateAndLinkNode();
			bNeedRefreshNode = true;
		}
	}
	if(bNeedRefreshNode)
	{
		MatEditorInterface->UpdateMaterialAfterGraphChange();
	}
	return FReply::Handled();
		
}

FReply SMatHelperWidget::InitialButton()
{
	const FMatHelperModule& MatHelper = FMatHelperModule::Get();
	for(auto Button : NodeButtons)
	{
		NodeButtonScrollBox->RemoveSlot(Button.ToSharedRef());
	}
	NodeButtons.Empty();

	const int32 Num = MatHelper.MatHelperMgn->NodeButtonInfo.Num();
	
	for(int i = 0 ; i < Num ; i++)
	{
		FNodeButton ButtonInfo = MatHelper.MatHelperMgn->NodeButtonInfo[i];
		TSharedPtr<SButton> Button = SNew(SButton)
		.Text(FText::FromString( ButtonInfo.ButtonName))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateRaw(this,&SMatHelperWidget::CreateMatNode,i));
		
		NodeButtonScrollBox->AddSlot()
		.Padding(3.0f)
		[
			Button.ToSharedRef()
		];
		
		NodeButtons.Add(Button);
	}
	
	
	return FReply::Handled();
}

FReply SMatHelperWidget::CreateMatNode(int32 Index) const
{
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
	const FString NodeFileName = PluginConfigPath + "AddNodeFile/" + MatHelper.MatHelperMgn->NodeButtonInfo[Index].ButtonName + ".txt";
	
	if(FPaths::FileExists(NodeFileName) == false)
	{
		return FReply::Handled();
	}
	
	FString NodeText;
	FFileHelper::LoadFileToString(NodeText,*NodeFileName);
	if(NodeText.Len() == 0)
	{
		
		MatHelper.EditorNotify("This Text Maybe Empty.",SNotificationItem::CS_Fail);
		return FReply::Handled();
	}
	
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	FPlatformApplicationMisc::ClipboardCopy(*NodeText);
	
	MatEditorInterface->FocusWindow();
	auto SelectedNodes = MatEditorInterface->GetSelectedNodes().Array();
	
	FVector2D RootOffset;
	if(MatHelper.MatHelperMgn->NodeButtonInfo[Index].RootOffsetOverride)
	{
		RootOffset = MatHelper.MatHelperMgn->NodeButtonInfo[Index].RootOffset;
	}
	else
	{
		RootOffset = MatHelper.MatHelperMgn->RootOffset;
	}

	const FVector2D BaseOffset = MatHelper.MatHelperMgn->BaseOffset;

	const UMaterialGraph* Graph = MatEditorInterface->GetMaterialInterface()->GetMaterial()->MaterialGraph;
	if(SelectedNodes.Num() > 0)
	{
		UObject* SelectedNode = SelectedNodes[0];
		if(const auto RootNode = Cast<UMaterialGraphNode_Root>(SelectedNode))
		{
			const FVector2D Location = FVector2D(RootNode->NodePosX + RootOffset.X,RootNode->NodePosY + RootOffset.Y);
			MatEditorInterface->PasteNodesHere(Location);
		}
		else if(const auto BaseNode = Cast<UMaterialGraphNode>(SelectedNode))
		{
			const FVector2D Location = FVector2D(BaseNode->NodePosX + BaseOffset.X,BaseNode->NodePosY + BaseOffset.Y);
			MatEditorInterface->PasteNodesHere(Location);
		}
	}
	else
	{
		const auto BaseRootNode =  Graph->RootNode;
		const FVector2D Location = FVector2D(BaseRootNode->NodePosX + RootOffset.X,BaseRootNode->NodePosY + RootOffset.Y);
		MatEditorInterface->PasteNodesHere(Location);
	}
	
	auto NewNodes = MatEditorInterface->GetSelectedNodes().Array();
	
	for(const auto Node : NewNodes)
	{
		UMaterialGraphNode* MatNode = Cast<UMaterialGraphNode>(Node);
		if(Cast<UMaterialExpressionMaterialFunctionCall>(MatNode->MaterialExpression))
		{
			MatNode->RecreateAndLinkNode();
		}
		MatEditorInterface->AddToSelection(MatNode->MaterialExpression);
	}

	const TSharedPtr<SGraphEditor> GraphEdPtr = MatEditorInterface->GraphEditor;
	if (GraphEdPtr.IsValid())
	{
		GraphEdPtr.Get()->JumpToNode(Cast<UMaterialGraphNode>(NewNodes[0]),false);
	}
	
	
	FPlatformApplicationMisc::ClipboardCopy(*ClipboardContent);
	return FReply::Handled();
}


FReply SMatHelperWidget::RefreshButton()
{
	InitialButton();
	return FReply::Handled();
}

bool ModifyName(FString& Name)
{
	const FString VersionsToReplace[] ={ TEXT(" (V2)"), TEXT(" (V3)"), TEXT(" (V4)"), TEXT(" (S)"), TEXT(" (T2d)"), TEXT(" (SB)")};
	
	for (auto& Version : VersionsToReplace)
	{
		if (Name.Contains(Version))
		{
			Name.ReplaceInline(*Version, TEXT(""));
			return true;
		}
	}
	return false; 
}

FReply SMatHelperWidget::RemoveParameterType() const
{
	bool ShouldRefresh = false;

	const auto GraphEdPtr = MatEditorInterface->GraphEditor;
	if (const auto GraphEd = GraphEdPtr.Get()) {
	    GraphEd->SelectAllNodes();
	}
	
	auto SelectedNodes = MatEditorInterface->GetSelectedNodes().Array();
	if (SelectedNodes.Num() == 0) {
	    return FReply::Handled();
	}
	
	for (const auto Node : SelectedNodes)
	{
	    const auto MatNode = Cast<UMaterialGraphNode>(Node);
	    if (MatNode)
	    {
	       	if (CheckNode(MatNode) == false)
	       	{
	       	    continue;
	       	}

	       	const auto Parameter = Cast<UMaterialExpressionParameter>(MatNode->MaterialExpression);
	       	if (Parameter != nullptr)
	       	{
	       	    FString Name = Parameter->ParameterName.ToString();
	       	    if (ModifyName(Name))
	       	    {
	       	        ShouldRefresh = true;
	       	        Parameter->ParameterName = *Name;
	       	    }
	       	}
	    }
	}

	if (ShouldRefresh) {
	    MatEditorInterface->UpdateMaterialAfterGraphChange();
	}
	return FReply::Handled();
}

void SMatHelperWidget::RefreshMaskPinSelection()
{
	const FMatHelperModule& MatHelper = FMatHelperModule::Get();
	TArray<FNodeMaskPin> Array = MatHelper.MatHelperMgn->MaskPinInfo;
	for(auto& Info : Array)
	{
		MaskPinOptions.Add(MakeShareable(new FString(Info.ButtonName)));
		MaskPinInfo.Add(Info.MaskValue);
	}
}

inline bool SMatHelperWidget::CheckNode(UObject* Node)
{
	bool CheckSuccess = true;
	if (Cast<UMaterialGraphNode_Comment>(Node)) { CheckSuccess = false; }
	if (Cast<UMaterialGraphNode_Root>(Node)) { CheckSuccess = false; }
	// UE4.26: UMaterialGraphNode_Composite / PinBase do not exist (UE5-only).
	if (Cast<UMaterialGraphNode_Knot>(Node)) { CheckSuccess = false; }
	return CheckSuccess;
}

#undef LOCTEXT_NAMESPACE