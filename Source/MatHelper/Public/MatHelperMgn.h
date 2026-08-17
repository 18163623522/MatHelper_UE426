// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "MatHelperMgn.generated.h"

#define DECLARE_NODE_MASK_PIN(Channel, R, G, B, A) FNodeMaskPin(FString(#Channel), FLinearColor(R, G, B, A))

UENUM()
enum class ESceneViewMethod : uint8
{
	// 创建SceneView时，自动设置为世界场景的视角.
	Auto,
	// 创建SceneView时，以选中的Actor为视角位置.
	SelectActor,
};

USTRUCT()
struct FNodeMaskPin
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere,Category = "Material")
	FString ButtonName = "";

	// UE4.26: FIntVector4 has no USTRUCT tag. Use FLinearColor (R,G,B,A = mask channels).
	UPROPERTY(EditAnywhere,Category = "Material")
	FLinearColor MaskValue = FLinearColor(0,0,0,0);

	FNodeMaskPin(){};
	FNodeMaskPin(const FString& InName, const FLinearColor InValue)
		:ButtonName(InName),MaskValue(InValue){}
};

USTRUCT()
struct FNodeButton
{
	GENERATED_BODY()

	// 在插件面板显示的名字，也是保存节点代码的文件名称.
	// 例如: 填写"ParticleColor"，插件面板的按钮名字会是 "ParticleColor"
	// 在配置文件夹里使用 ParticleColor.txt 保存这个节点的代码.
	UPROPERTY(EditAnywhere,Category = "Material")
	FString ButtonName = "None";

	// 覆盖RootOffset
	UPROPERTY(EditAnywhere,Category = "Material")
	bool RootOffsetOverride = false;

	//节点按钮创建节点时，将会使用这个Root偏移值.
	UPROPERTY(EditAnywhere,Category = "Material",meta = (EditCondition = RootOffsetOverride))
	FVector2D RootOffset = FVector2D(-200,160);

	FNodeButton(){}
	FNodeButton(const FString& InButtonName)
		:ButtonName(InButtonName){}
};

UCLASS()
class MATHELPER_API UMatHelperMgn : public UDataAsset
{
	UMatHelperMgn();
	GENERATED_BODY()
	
public:
	FString PluginButtonConfigPath;
	
	// 打开节点配置文件夹
	UFUNCTION(CallInEditor,Category = "Material")
	void OpenNodesConfigFolder();

	// 编辑节点文本
	UFUNCTION(CallInEditor,Category = "Material")
	void EditButtonInfo();

	// 刷新节点按钮
	UFUNCTION(CallInEditor,Category = "Material")
	void RefreshHelpersButton();

	// 重新启动引擎
	UFUNCTION(CallInEditor,Category = "Material")
	void RestartEditor();

	// ICON的SVG文件名称，例如:A.svg,只需要填写 A 即可.
	UPROPERTY(EditAnywhere,Category = "Material")
	FString IConName;

	// 应用ICON
	UFUNCTION(CallInEditor,Category = "Material")
	void ModifyICON();

public:
	// 创建SceneView时，选择视角的方式.
	UPROPERTY(EditAnywhere,Category = "General", meta = (DisplayName = "场景视图视角"))
	ESceneViewMethod SceneViewMethod = ESceneViewMethod::Auto;

public:
	//材质资产的显示颜色，引擎默认是绿色(64,192,64)
	UPROPERTY(EditAnywhere,Category = "Material", meta = (DisplayName = "材质资产颜色"))
	FColor MaterialAssetColor = FColor(255,25,25);

	//材质实例资产的显示颜色，引擎默认是绿色(0,128,0)
	UPROPERTY(EditAnywhere,Category = "Material", meta = (DisplayName = "材质实例资产颜色"))
	FColor MaterialInstanceAssetColor = FColor(0,128,0);

	// 决定了插件助手面板的占比，1是占一半，2是全占. 0.5是一半的一半
	// 更改后需要重新启动 材质编辑器 才会生效
	UPROPERTY(EditAnywhere,Category = "Material", meta = (DisplayName = "面板高度占比"))
	float HeightRatio = 1.0;

	// 节点按钮创建节点的位置---相对于Root根节点的位置偏移
	// 但可以勾选 RootOffsetOverride 覆盖这个值.
	UPROPERTY(EditAnywhere,Category = "Material", meta = (DisplayName = "根节点偏移"))
	FVector2D RootOffset = FVector2D(-100,800);

	// 节点按钮创建节点的位置---相对于普通节点的位置偏移
	UPROPERTY(EditAnywhere,Category = "Material", meta = (DisplayName = "基础节点偏移"))
	FVector2D BaseOffset = FVector2D(50,50);

	UPROPERTY(EditAnywhere,Category = "Material", meta = (DisplayName = "Mask 引脚列表"))
	TArray<FNodeMaskPin> MaskPinInfo =
		{
			DECLARE_NODE_MASK_PIN(R, 1, 0, 0, 0),
			DECLARE_NODE_MASK_PIN(G, 0, 1, 0, 0),
			DECLARE_NODE_MASK_PIN(B, 0, 0, 1, 0),
			DECLARE_NODE_MASK_PIN(A, 0, 0, 0, 1),
			DECLARE_NODE_MASK_PIN(RGB, 1, 1, 1, 0),
			DECLARE_NODE_MASK_PIN(RGBA, 1, 1, 1, 1),
			DECLARE_NODE_MASK_PIN(RG, 1, 1, 0, 0),
			DECLARE_NODE_MASK_PIN(BA, 0, 0, 1, 1)
		};
	
	// 自动分组关键词
	// UE4.26: starter keywords so 自动分组 works before the user edits the DataAsset.
	UPROPERTY(EditAnywhere,Category = "Material", meta = (DisplayName = "自动分组关键词"))
	TArray<FString> AutoGroupKeys =
		{
			TEXT("BaseColor"),
			TEXT("Color"),
			TEXT("Normal"),
			TEXT("Roughness"),
			TEXT("Metallic"),
			TEXT("Specular"),
			TEXT("Emissive"),
			TEXT("Opacity"),
			TEXT("AO"),
			TEXT("Mask"),
			TEXT("_R"),
			TEXT("_G"),
			TEXT("_B"),
			TEXT("_A")
		};

	// 节点按钮信息
	// UE4.26: built-in defaults so the panel works without rebuilding the DataAsset
	// (the shipped uasset is UE5 format and cannot load in 4.26). ButtonName must
	// match a Config/AddNodeFile/<ButtonName>.txt template.
	UPROPERTY(EditAnywhere,Category = "Material", meta = (DisplayName = "节点按钮信息"))
	TArray<FNodeButton> NodeButtonInfo =
		{
			FNodeButton(TEXT("Fresnel")),
			FNodeButton(TEXT("ParticleColor"))
		};
	
public:
	
	// 将Niagara拖入定序器时，自动打开轨道并设置为 DesiredAge 模式.
	// 设置更改后需要重启引擎.
	UPROPERTY(EditAnywhere,Category = "Niagara", meta = (DisplayName = "覆盖 Niagara 定序器模式"))
	bool OverrideNiagaraSequenceMode = true;

	UPROPERTY(EditAnywhere,Category = "Niagara", meta = (DisplayName = "创建 Niagara 自动播放选中"))
	bool CreateNiagaraAutoPlaySelection = true;

private:
	FSlateBrush* CreateHeaderBrush();
};