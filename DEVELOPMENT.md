# MatHelper UE 4.26 开发文档

> 本文档面向二次开发者，详细说明 MatHelper UE 4.26 移植版的架构、模块、API 映射、构建流程和调试指南。

---

## 目录

1. [项目概览](#1-项目概览)
2. [目录结构](#2-目录结构)
3. [模块架构](#3-模块架构)
4. [类与职责详解](#4-类与职责详解)
5. [UE 5.3 → 4.26 API 映射表](#5-ue-53--426-api-映射表)
6. [移植决策记录](#6-移植决策记录)
7. [构建与部署](#7-构建与部署)
8. [调试指南](#8-调试指南)
9. [已知限制与待办](#9-已知限制与待办)
10. [二次开发指南](#10-二次开发指南)

---

## 1. 项目概览

| 属性 | 值 |
|---|---|
| 原始版本 | [AKaKLya/MatHelper](https://github.com/AKaKLya/MatHelper) (UE 5.3) |
| 移植目标 | Unreal Engine 4.26 |
| 插件类型 | Editor-only (Win64) |
| 模块数 | 1 (`MatHelper`) |
| 源码行数 | ~2819 行（含 .h/.cpp/.cs/.inl） |
| 依赖插件 | Niagara |
| 编译状态 | ✅ 通过（UE 4.26 + VS2019 工具链） |
| GitHub 仓库 | https://github.com/18163623522/MatHelper_UE426 |

### 功能清单

| 功能 | 状态 | 说明 |
|---|---|---|
| 材质编辑器助手面板 | ✅ | SMatHelperWidget，嵌入材质编辑器 Palette |
| 自定义节点按钮 | ✅ | Config/AddNodeFile/*.txt 文本模板创建节点 |
| 资产图标颜色区分 | ✅ | 材质红、实例绿（FAsssetTypeActions::GetTypeColor） |
| Niagara 播放按钮 | ✅ | Tag 检测机制，支持编辑器工具栏/材质/实例编辑器 |
| 资产 Lock/UnLock | ✅ | PKG_DisallowExport 标记 |
| SceneView 场景预览 | ✅ | 9 个全局 + 编辑器内嵌 |
| 中英语言切换 | ✅ | UKismetInternationalizationLibrary |
| 重启引擎 | ✅ | FUnrealEdMisc::RestartEditor |
| 常用地图快速打开 | ✅ | 下拉菜单，配置在 UMatHelperSettings |
| 世界大纲 Niagara Lock 列 | ✅ | FOuterlineSelectionLockCol |
| 材质实例参数批量开关 | ✅ | SMatInstanceHelper（Enable/Disable Params） |
| Niagara Sequencer 自动轨道 | ❌ | 4.26 ILevelSequenceModule 无 OnNewActorTrackAdded |
| SVG Slate 图标 | ❌ | 4.26 不支持 FSlateVectorImageBrush |
| Material Layers 图层树 | ❌ | UE5 专属 API，已删除（原为死代码） |

---

## 2. 目录结构

```
MatHelper_426/
├── MatHelper.uplugin              # 插件描述符（EngineVersion: 4.26.0）
├── README.md                      # 项目说明
├── DEVELOPMENT.md                 # 本开发文档
│
├── Config/
│   ├── AddNodeFile/               # 节点文本模板（跨版本通用）
│   │   ├── Fresnel.txt
│   │   └── ParticleColor.txt
│   └── FilterPlugin.ini           # 打包过滤配置
│
├── Content/                       # ⚠️ UE5 格式 uasset，需在 4.26 重建
│   ├── MatHelper.uasset           # UMatHelperMgn DataAsset（插件配置）
│   ├── Material/
│   │   ├── MI_Empty.uasset        # 空材质实例模板（CreateInstance 用）
│   │   └── M_SlateIcon.uasset     # 图标材质
│
├── Resources/
│   ├── Icon128.png                # 插件图标
│   ├── NiagaraIcon.png            # Niagara 按钮图标
│   ├── PlaceholderButtonIcon.svg  # 占位图标（4.26 不加载 SVG）
│   └── Graph/                     # 节点图标 SVG 集合
│
└── Source/MatHelper/
    ├── MatHelper.Build.cs         # 模块构建规则
    │
    ├── Public/                    # 头文件
    │   ├── MatHelper.h            # FMatHelperModule（主模块类）
    │   ├── MatHelperMgn.h         # UMatHelperMgn（配置 DataAsset）
    │   ├── MatHelperSettings.h    # UMatHelperSettings（编辑器设置）
    │   ├── MatHelperWidget.h      # SMatHelperWidget（助手面板）
    │   ├── MatInstanceHelper.h    # SMatInstanceHelper（参数开关）
    │   ├── ButtonInfoEditor.h     # SButtonInfoEditor（节点编辑器）
    │   ├── SceneEditorView.h      # SceneEditorView（场景预览）
    │   ├── SceneEditorViewToolBar.h
    │   ├── OuterlineSelectionCol.h # 世界大纲 Lock 列
    │   ├── TAccessPrivate.inl     # 私有成员访问模板
    │   ├── ButtonClass/
    │   │   ├── SimpleButtonCommands.h  # UI 命令注册
    │   │   └── SimpleButtonStyle.h     # Slate 样式
    │   └── EngineClass/
    │       ├── CusAssetTypeActions_Material.h      # 材质资产操作
    │       ├── CusAssetTypeActions_MatInstance.h   # 材质实例资产操作
    │       └── CusAssetTypeActions_MatInterface.h  # 材质接口资产操作
    │
    └── Private/                   # 实现文件
        ├── MatHelper.cpp          # 模块核心实现（699 行）
        ├── MatHelperMgn.cpp       # 配置管理实现
        ├── MatHelperWidget.cpp    # 助手面板实现（680 行）
        ├── MatInstanceHelper.cpp  # 参数开关实现
        ├── ButtonInfoEditor.cpp   # 节点编辑器实现
        ├── SceneEditorView.cpp    # 场景预览实现
        ├── SceneEditorViewToolBar.cpp
        ├── OuterlineSelectionCol.cpp
        ├── MatHelperSettings.cpp
        ├── ButtonClass/
        │   ├── SimpleButtonCommands.cpp
        │   └── SimpleButtonStyle.cpp
        └── EngineClass/
            ├── CusAssetTypeActions_Material.cpp
            ├── CusAssetTypeActions_MatInstance.cpp
            └── CusAssetTypeActions_MatInterface.cpp
```

---

## 3. 模块架构

### 模块依赖图

```
MatHelper (Editor)
├── Core / CoreUObject / Engine
├── Slate / SlateCore / ApplicationCore / InputCore
├── UnrealEd (FEditorViewportClient, AssetSelection, Thumbnail)
├── MaterialEditor (IMaterialEditor, FMaterialEditor, FMaterialInstanceEditor, SMaterialPalette)
├── GraphEditor (SGraphEditor, SGraphPalette)
├── AssetTools (FAsssetTypeActions_Base, IAssetTools)
├── ContentBrowser (IContentBrowserSingleton)
├── ToolMenus (UToolMenus)
├── EditorStyle (FEditorStyle)
├── PropertyEditor (IDetailCustomization)
├── SceneOutliner (ISceneOutlinerColumn)
├── LevelEditor / LevelSequence / Sequencer / MovieScene
├── NiagaraEditor / Niagara
├── StaticMeshEditor / RenderCore
├── DeveloperSettings (UDeveloperSettings)
├── Kismet (UGameplayStatics, UKismetInternationalizationLibrary)
├── AssetRegistry
└── Projects (IPluginManager)
```

### 启动流程

```
FMatHelperModule::StartupModule()
├── InitPluginInfo()
│   ├── 加载 UMatHelperMgn DataAsset（/MatHelper/MatHelper.MatHelper）
│   ├── FSimpleButtonStyle::Initialize()  — 注册 Slate 样式
│   ├── FSimpleButtonCommands::Register() — 注册 UI 命令
│   └── UToolMenus::RegisterStartupCallback → RegisterButton()
│
├── RegisterTab()
│   └── 注册 13 个 NomadTabSpawner（ButtonInfoEditor + 12 个 SceneView）
│
├── InitMatEditorHook()
│   ├── 注册 3 个 FAssetTypeActions（材质/实例/接口）
│   ├── OnMaterialEditorOpened delegate
│   │   └── 在 Palette 面板插入 SMatHelperWidget
│   └── OnMaterialInstanceEditorOpened delegate
│       └── 注册 SceneView tab + 工具栏扩展
│
├── InitNiagaraEditorHook()
│   ├── Niagara 编辑器工具栏扩展（Play Niagara 按钮）
│   └── RegisterNiagaraAutoPlayer()（如果配置开启）
│       ├── LevelEditor / MaterialEditor / MaterialInstanceEditor 工具栏按钮
│       └── SceneOutliner Lock 列注册
│
└── RegisterGameEditorMenus()
    └── LevelEditor 工具栏 Common Maps 下拉
```

### 关闭流程

```
FMatHelperModule::ShutdownModule()
├── 移除 MaterialEditor delegate
└── 注销 3 个 FAssetTypeActions
```

---

## 4. 类与职责详解

### 4.1 FMatHelperModule

**文件**: `MatHelper.h` / `MatHelper.cpp`
**职责**: 插件主模块，统筹所有功能注册和生命周期管理。

**关键方法**:

| 方法 | 职责 |
|---|---|
| `StartupModule()` | 注册 tab、hook 编辑器、注册菜单 |
| `ShutdownModule()` | 移除 delegate、注销 AssetTypeActions |
| `InitMatEditorHook()` | 注册 AssetTypeActions、hook 材质/实例编辑器打开事件 |
| `InitNiagaraEditorHook()` | Niagara 编辑器工具栏扩展 |
| `RegisterButton()` | Tools 菜单注册（MatHelper/SceneView/Lock/UnLock/Restart） |
| `RegisterNiagaraAutoPlayer()` | Niagara 播放按钮 + 大纲 Lock 列 |
| `ToggleAssetFlag(bIsLock)` | 资产 Lock/UnLock（PKG_DisallowExport） |
| `PlayNiagaraOnEditorWorld()` | 播放带 NiagaraAutoPlay Tag 的 Niagara Actor |
| `EditorNotify()` | 编辑器通知消息 |

**静态成员**:

| 成员 | 用途 |
|---|---|
| `ButtonInfoEditorTabName` | 节点编辑器 tab 名 |
| `SceneViewEditorTabName` | 全局 SceneView tab 名 |
| `MaterialSceneViewEditorTabName` | 材质编辑器内 SceneView tab 名 |

### 4.2 UMatHelperMgn

**文件**: `MatHelperMgn.h` / `MatHelperMgn.cpp`
**基类**: `UDataAsset`
**职责**: 插件配置数据资产，存储所有可编辑配置。

**配置属性**:

| 属性 | 类型 | 说明 |
|---|---|---|
| `MaterialAssetColor` | `FColor` | 材质资产图标颜色（默认红 255,25,25） |
| `MaterialInstanceAssetColor` | `FColor` | 材质实例图标颜色（默认绿 0,128,0） |
| `HeightRatio` | `float` | 助手面板占比（1=一半，2=全占） |
| `RootOffset` | `FVector2D` | 节点创建相对 Root 偏移 |
| `BaseOffset` | `FVector2D` | 节点创建相对普通节点偏移 |
| `MaskPinInfo` | `TArray<FNodeMaskPin>` | Mask Pin 配置（R/G/B/A/RGB/RGBA/RG/BA） |
| `AutoGroupKeys` | `TArray<FString>` | 自动分组关键词 |
| `NodeButtonInfo` | `TArray<FNodeButton>` | 节点按钮信息 |
| `SceneViewMethod` | `ESceneViewMethod` | SceneView 视角方式（Auto/SelectActor） |
| `OverrideNiagaraSequenceMode` | `bool` | ⚠️ 4.26 无效（Niagara 自动轨道已删除） |
| `CreateNiagaraAutoPlaySelection` | `bool` | 是否创建 Niagara 播放选择 |

**CallInEditor 方法**:

| 方法 | 职责 |
|---|---|
| `OpenNodesConfigFolder()` | 打开节点配置文件夹 |
| `EditButtonInfo()` | 打开节点编辑器 tab |
| `RefreshHelpersButton()` | 刷新所有助手面板按钮 |
| `RestartEditor()` | 重启编辑器 |
| `ModifyICON()` | 修改插件图标 |

### 4.3 SMatHelperWidget

**文件**: `MatHelperWidget.h` / `MatHelperWidget.cpp`
**基类**: `SCompoundWidget`
**职责**: 材质编辑器助手面板，嵌入 Palette 左侧。

**功能按钮**:

| 按钮 | 方法 | 说明 |
|---|---|---|
| MatHelper Manager | lambda | 打开 UMatHelperMgn 编辑器 |
| Scene View | lambda | 打开材质编辑器内 SceneView |
| Set Group | `SetNodeGroup(false, false)` | 设置选中节点分组 |
| Auto Group | `SetNodeGroup(true, false)` | 按关键词自动分组 |
| Auto All Group | `SetNodeGroup(true, true)` | 全选后自动分组 |
| Add Mask Pin | `AddNodeMaskPin()` | 添加/删除 Mask Pin |
| Show Pin Name | lambda | 切换 Pin 名显示 |
| Create Instance | `CreateInstance()` | 创建材质实例 |
| Refraction | `ToggleRefraction()` | 切换折射模式 |
| Fix Function Node | `FixFunctionNode()` | 修复函数节点 |
| Auto Name | `RemoveParameterType()` | 去除参数类型后缀 |
| 动态按钮 | `CreateMatNode(Index)` | 从 txt 模板创建节点 |

**私有成员访问**: 通过 `#define private public` hack 访问 `FMaterialEditor::GraphEditor`（private TSharedPtr<SGraphEditor>）。

### 4.4 SMatInstanceHelper

**文件**: `MatInstanceHelper.h` / `MatInstanceHelper.cpp`
**基类**: `SScrollBox`
**职责**: 材质实例参数批量开关（Enable/Disable Params）。

**工作原理**: 创建临时 `UMaterialEditorInstanceConstant`，遍历所有参数组调用 `FMaterialPropertyHelpers::OnOverrideParameter`。

### 4.5 FAssetTypeActions 系列

| 类 | 支持类型 | 颜色 | 职责 |
|---|---|---|---|
| `FCusAssetTypeActions_Material` | `UMaterial` | 红 (255,25,25) | 覆盖材质资产颜色、打开编辑器、Thumbnail |
| `FCusAssetTypeActions_MaterialInstanceConstant` | `UMaterialInstanceConstant` | 绿 (0,128,0) | 覆盖实例颜色、打开 MIC 编辑器 |
| `FCusAssetTypeActions_MaterialInterface` | `UMaterialInterface` | 绿 (64,192,64) | 基础材质接口 |

**注册方式**:
```cpp
IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
MaterialAssetTypeActions = MakeShareable(new FCusAssetTypeActions_Material());
MaterialAssetTypeActions->SetAssetColor(MatHelperMgn->MaterialAssetColor);
AssetTools.RegisterAssetTypeActions(MaterialAssetTypeActions.ToSharedRef());
```

### 4.6 SceneEditorView

**文件**: `SceneEditorView.h` / `SceneEditorView.cpp`
**基类**: `SEditorViewport` + `ICommonEditorViewportToolbarInfoProvider`
**职责**: 独立场景预览窗口，可在任意编辑器中打开。

**视角模式**:
- `Auto`: 复制主编辑器视口位置
- `SelectActor`: 跳转到选中 Actor 位置（用 `GEditor->GetSelectedActors()`）

### 4.7 FOuterlineSelectionLockCol

**文件**: `OuterlineSelectionCol.h` / `OuterlineSelectionCol.cpp`
**基类**: `ISceneOutlinerColumn`
**职责**: 世界大纲中添加 Niagara Lock 复选框列。

**工作原理**: 检查 Actor 是否为 `ANiagaraActor`，复选框状态绑定 `NiagaraAutoPlay` Tag。

### 4.8 TAccessPrivate

**文件**: `TAccessPrivate.inl`
**职责**: 访问 C++ private/protected 成员的模板工具。

**4.26 适配**: 原 UE5 版用 `inline static`（C++17），4.26 改为函数局部静态变量 `Get()`。实际使用中大部分被 `#define` hack 替代，仅保留框架。

### 4.9 FCompoundWidgetAccessor

**文件**: `MatHelper.cpp`（文件作用域）
**职责**: 暴露 `SCompoundWidget::ChildSlot`（protected）为 public。

```cpp
class FCompoundWidgetAccessor : public SCompoundWidget
{
public:
    using SCompoundWidget::ChildSlot;
};
```

通过 `reinterpret_cast` 访问 `SMaterialPalette` 的 `ChildSlot`。

---

## 5. UE 5.3 → 4.26 API 映射表

### 架构层

| UE 5.3 | UE 4.26 | 处理方式 |
|---|---|---|
| `UAssetDefinition` / `UAssetDefinitionDefault` | `FAsssetTypeActions_Base` | 3 个类完全重写 |
| `UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass()` | `IAssetTools::RegisterAssetTypeActions()` | 注册自己的 ActionTypeActions 覆盖 |
| `EAssetCommandResult` / `FAssetOpenArgs` | `OpenAssetEditor(const TArray<UObject*>&, TSharedPtr<IToolkitHost>)` | 改签名 |
| `EAssetCategoryPaths::Material` | `EAssetTypeCategories::MaterialsAndTextures` | 改枚举 |
| `AssetDefinition` 模块 | 无 | Build.cs 删除依赖 |
| `EngineAssetDefinitions` 插件 | 无 | uplugin 删除依赖 |

### 样式层

| UE 5.3 | UE 4.26 | 处理方式 |
|---|---|---|
| `FAppStyle::GetAppStyleSetName()` | `FEditorStyle::GetStyleSetName()` | 全局替换（~20 处） |
| `FAppStyle::Get()` | `FEditorStyle::Get()` | 返回 `ISlateStyle&` |
| `FAppStyle::GetFontStyle()` | `FEditorStyle::GetFontStyle()` | 直接替换 |
| `IMAGE_BRUSH_SVG` 宏 | 无 | 4.26 不支持 SVG，改用 `FSlateBrush` |
| `SlateStyleMacros.h` | 无 | 4.26 无此头文件 |
| `FVector2f` | `FVector2D` | 4.26 用 double 版本 |
| `FStyleColors::White` | `FLinearColor::White` | 直接替换 |

### 设置层

| UE 5.3 | UE 4.26 | 处理方式 |
|---|---|---|
| `UDeveloperSettingsBackedByCVars` | `UDeveloperSettings` | 4.26 已内置 CVar 支持 |
| `#include "Engine/DeveloperSettingsBackedByCVars.h"` | `#include "Engine/DeveloperSettings.h"` | 改 include |

### 菜单与工具栏

| UE 5.3 | UE 4.26 | 处理方式 |
|---|---|---|
| `AssetEditor.MaterialEditor.ToolBar` | `AssetEditor.MaterialEditorApp.ToolBar` | AppIdentifier 带后缀 |
| `AssetEditor.MaterialInstanceEditor.ToolBar` | `AssetEditor.MaterialInstanceEditorApp.ToolBar` | 同上 |
| `FToolMenuEntry::StyleNameOverride` | 无 | 删除该行 |
| `MainFrame.MainMenu.Tools` | 不变 | 兼容 |
| `LevelEditor.LevelEditorToolBar.PlayToolBar` | 不变 | 兼容 |

### 编辑器子系统

| UE 5.3 | UE 4.26 | 处理方式 |
|---|---|---|
| `UEditorAssetSubsystem::DoesAssetExist()` | `LoadObject<UObject>() != nullptr` | 手动检查 |
| `UEditorAssetSubsystem::DuplicateAsset()` | `DuplicateObject()` + `SavePackage()` | 手动复制 |
| `UEditorActorSubsystem::GetSelectedLevelActors()` | `GEditor->GetSelectedActors()->GetSelectedObjects<AActor>()` | 用 USelection |
| `AssetViewUtils::OpenEditorForAsset()` | `UAssetEditorSubsystem::OpenEditorForAsset()` | 改用公开 API |
| `UToolMenus::Get()` 返回引用 | 返回 `UToolMenus*` | 注意指针语义 |

### 材质编辑器

| UE 5.3 | UE 4.26 | 处理方式 |
|---|---|---|
| `FMaterialEditor::FocusedGraphEdPtr` (TWeakPtr) | `FMaterialEditor::GraphEditor` (TSharedPtr) | 改成员名+类型 |
| `FMaterialEditor::OriginalMaterialObject` | `GetMaterialInterface()->GetMaterial()` | 用接口方法 |
| `SCompoundWidget::FCompoundWidgetOneChildSlot` | `FSimpleSlot` | 改类型名 |
| `UMaterial::RefractionMethod` / `RM_None` | `UMaterial::RefractionMode` / 无 `RM_None` | 在 `RM_IndexOfRefraction`/`RM_PixelNormalOffset` 间切换 |
| `UMaterialGraphNode_Composite` | 不存在 | 删除 |
| `UMaterialGraphNode_PinBase` | 不存在 | 删除 |
| `UE::Editor::FindOrCreateThumbnailInfo<T>()` | 手动 `NewObject<USceneThumbnailInfoWithPrimitive>` | 复制 4.26 引擎模式 |
| `USceneThumbnailInfoWithPrimitive::DefaultPrimitiveType` | `PrimitiveType` | 改成员名 |

### SceneOutliner

| UE 5.3 | UE 4.26 | 处理方式 |
|---|---|---|
| `FSceneOutlinerTreeItemRef` / `FSceneOutlinerTreeItemPtr` | `SceneOutliner::FTreeItemRef` / `SceneOutliner::FTreeItemPtr` | 加命名空间 |
| `ISceneOutlinerTreeItem.h` | `ITreeItem.h` | 改 include |
| `TreeItem->CastTo<FActorTreeItem>()` | `StaticCastSharedRef<SceneOutliner::FActorTreeItem>(TreeItem)` | 改转换方式 |
| `FActorTreeItem::IsValid()` | `Actor.IsValid()` | 改检查方式 |
| `FSceneOutlinerColumnInfo(visibility, priority, factory)` | `FDefaultColumnInfo(FColumnInfo(...))` | 嵌套构造 |

### Niagara

| UE 5.3 | UE 4.26 | 处理方式 |
|---|---|---|
| `FNiagaraEditorModule::DefaultTrackHandle` | `CreateSystemTrackEditorHandle` | 改成员名（最终删除该功能） |
| `ILevelSequenceModule::OnNewActorTrackAdded()` | 不存在 | 删除 Niagara 自动轨道功能 |
| `UMovieSceneNiagaraSystemSpawnSection::AgeUpdateMode` (private) | 有 `GetAgeUpdateMode()` getter | 模板黑魔法（最终删除） |

### C++ 标准兼容

| UE 5.3 (C++17) | UE 4.26 (C++14) | 处理方式 |
|---|---|---|
| `inline static const FName X = "..."` | `static const FName X;` + cpp 定义 | 声明在 h，定义在 cpp |
| `inline static typename T::Type Value` | `static T::Type& Get() { static T::Type V; return V; }` | 函数局部静态 |
| `if (auto* x = Cast<T>(...); cond)` | `auto* x = Cast<T>(...); if (cond)` | 拆开 if-init |
| `TObjectPtr<T>` | `T*` | 4.26 不支持 TObjectPtr |
| `FIntVector4` (USTRUCT) | `FLinearColor` | 4.26 FIntVector4 无 USTRUCT 标记 |

### 私有成员访问

| UE 5.3 | UE 4.26 | 处理方式 |
|---|---|---|
| 模板黑魔法 `TAccessPrivateStub` | `#define private public` hack | PCH 限制导致模板失效 |
| `&FMaterialEditor::Palette` (private) | `#define private public` + 直接访问 | 包裹 include |
| `&SCompoundWidget::ChildSlot` (protected) | `FCompoundWidgetAccessor` 派生类 + `reinterpret_cast` | 布局兼容转换 |

---

## 6. 移植决策记录

### 6.1 为什么删除 Detail Customization？

**发现**: 原插件有 3 个 `CusMaterialEditorInstanceDetailCustomization` 版本文件（主/53/54，共 5222 行），但 `FCusMaterialInstanceParameterDetails` 类**从未被任何代码注册或调用**——它是死代码。

**验证**: `grep -rn "FCusMaterialInstanceParameterDetails" --exclude="Customization*.cpp" --exclude="Customization*.h"` 返回空。

**结论**: 参数批量开关功能实际由 `SMatInstanceHelper`（MatInstanceHelper.cpp，51 行）独立实现，不依赖 Detail Customization。安全删除全部 5222 行死代码。

### 6.2 为什么删除 Niagara 自动轨道？

**原因**: 4.26 的 `ILevelSequenceModule` 只有 `RegisterObjectSpawner`/`UnregisterObjectSpawner`，没有 `OnNewActorTrackAdded` delegate。原插件通过这个 delegate 在 Niagara Actor 拖入 Sequencer 时自动添加 System Life Cycle 轨道并设为 DesiredAge 模式。

**影响**: `OverrideNiagaraSequenceMode` 配置项在 4.26 无效（保留在配置中但不生效）。Niagara 播放按钮功能不受影响。

### 6.3 为什么用 `#define` hack 而不是模板黑魔法？

**原因**: 4.26 默认 C++14，MSVC 14.44 对 `&Class::PrivateMember` 在模板显式实例化参数中的访问检查更严格，模板黑魔法编译失败（C2248）。

**替代方案**: `#define private public` / `#define protected public` 在 include 引擎头文件前生效，预处理阶段取消访问控制。这是常见的 UE 插件技巧。

**限制**: PCH（预编译头）已经处理过的头文件不受 `#define` 影响。`SCompoundWidget::ChildSlot` 在 PCH 里已经是 protected，所以额外用了 `FCompoundWidgetAccessor` 派生类 + `reinterpret_cast`。

### 6.4 为什么 `FIntVector4` 改成 `FLinearColor`？

**原因**: 4.26 的 `FIntVector4` 存在但没有 `USTRUCT` 标记，UHT（UnrealHeaderTool）拒绝它作为 `UPROPERTY` 类型。

**替代**: `FLinearColor` 有 `USTRUCT` 标记，RGBA 四个 float 对应 mask 的 R/G/B/A，值 0/1 足够表达 mask 开关。

---

## 7. 构建与部署

### 环境要求

- Unreal Engine 4.26（本机: `E:\Softwave\UE\UE_4.26`）
- Visual Studio 2019 工具链（或 VS2022 BuildTools）
- Windows 10 SDK
- Niagara 插件（引擎自带）

### 部署步骤

1. 复制 `MatHelper_426/` 到 UE 4.26 项目的 `Plugins/` 目录
2. 在 `.uproject` 文件中添加:
   ```json
   "Plugins": [
     { "Name": "MatHelper", "Enabled": true },
     { "Name": "Niagara", "Enabled": true }
   ]
   ```
3. 生成项目文件: 右键 `.uproject` → Generate Visual Studio project files
4. 编译: 打开 `.sln` → Build (Development Editor)

### 命令行编译

```bash
# UE4.26 安装路径
UE426="E:/Softwave/UE/UE_4.26"

# 编译
"$UE426/Engine/Build/BatchFiles/Build.bat" YourProjectEditor Win64 Development \
  -Project="Path/To/YourProject.uproject" -WaitMutex -FromMsBuild
```

### Content 资源处理

`Content/` 下的 uasset 是 UE5 序列化格式，4.26 无法直接加载。需要重建:

1. **MatHelper.uasset** (UMatHelperMgn DataAsset): 在 4.26 编辑器中新建 DataAsset，配置 `NodeButtonInfo`、`MaskPinInfo` 等属性
2. **MI_Empty.uasset** (空材质实例): 创建一个 `UMaterialInstanceConstant`，Parent 设为 None
3. **M_SlateIcon.uasset** (图标材质): 可选，用于 `ModifyICON` 功能

---

## 8. 调试指南

### 常见编译错误

#### C2248: 无法访问 protected/private 成员

**原因**: 访问了引擎类的非公开成员。

**解决**:
- 确认 `#define private public` / `#define protected public` 在 include 之前
- 对于 PCH 已处理的类（如 `SCompoundWidget`），用派生类 + `reinterpret_cast`

#### C7525: 内联变量至少需要 /std:c++17

**原因**: 4.26 默认 C++14，不支持 `inline static`。

**解决**: 改为头文件声明 + cpp 文件定义。

#### C1083: 无法打开包括文件

**原因**: UE5 专属头文件在 4.26 不存在。

**常见不存在文件**:
- `SlateStyleMacros.h`
- `MaterialGraph/MaterialGraphNode_Composite.h`
- `MaterialGraph/MaterialGraphNode_PinBase.h`
- `ISceneOutlinerTreeItem.h` → 用 `ITreeItem.h`
- `Subsystems/EditorAssetSubsystem.h`

### 运行时调试

#### 插件未加载

检查 `MatHelper.uplugin` 的 `EngineVersion` 是否为 `4.26.0`，`Modules` 的 `Type` 是否为 `Editor`。

#### 助手面板不显示

- 确认 `UMatHelperMgn` DataAsset 已正确加载（`LoadObject` 路径 `/MatHelper/MatHelper.MatHelper`）
- 如果 Content 资源未重建，`MatHelperMgn` 为 nullptr，会导致助手面板无法初始化
- 检查 `OnMaterialEditorOpened` delegate 是否绑定成功

#### 节点按钮不工作

- 确认 `Config/AddNodeFile/*.txt` 文件存在且非空
- 检查 `UMatHelperMgn::NodeButtonInfo` 配置是否正确
- 节点创建通过剪贴板粘贴机制，确保剪贴板未被其他程序占用

#### Niagara 播放无效

- 确认 Niagara Actor 有 `NiagaraAutoPlay` Tag
- 检查世界大纲 Lock 列是否正常显示复选框

### 日志

插件没有自定义日志类别。如需添加，在 `MatHelper.cpp` 顶部:

```cpp
DEFINE_LOG_CATEGORY_STATIC(LogMatHelper, Log, All);
```

使用:
```cpp
UE_LOG(LogMatHelper, Warning, TEXT("MatHelper: %s"), *SomeString);
```

---

## 9. 已知限制与待办

### 已知限制

| 限制 | 原因 | 影响 |
|---|---|---|
| Content/ uasset 无法直接使用 | UE5 序列化格式 | 需在 4.26 重建 DataAsset |
| SVG 图标不显示 | 4.26 无 `FSlateVectorImageBrush` | 按钮用纯色 brush，不影响功能 |
| Niagara 自动轨道不可用 | 4.26 无 `OnNewActorTrackAdded` | Niagara 拖入 Sequencer 不会自动添加轨道 |
| `OverrideNiagaraSequenceMode` 配置无效 | 同上 | 配置项保留但不生效 |
| 折射切换无 `RM_None` | 4.26 枚举只有两个值 | 在 `RM_IndexOfRefraction`/`RM_PixelNormalOffset` 间切换 |

### 待办

- [ ] 在 4.26 编辑器内运行测试，验证所有功能
- [ ] 重建 Content/ 资源（MatHelper.uasset / MI_Empty.uasset / M_SlateIcon.uasset）
- [ ] 添加自定义日志类别 `LogMatHelper`
- [ ] 考虑用 PNG 替代 SVG 图标（4.26 支持 `FSlateImageBrush` 加载 PNG）
- [ ] 考虑用 `IAssetTools::CreateAsset` 替代 `DuplicateObject` + `SavePackage`（更标准）

---

## 10. 二次开发指南

### 添加新的节点按钮

1. 在 `Config/AddNodeFile/` 创建 `YourNode.txt`，内容为材质节点的 T3D 文本
   - 从材质编辑器中复制节点，粘贴到文本文件
   - 清理 `ExportPath` 等路径相关属性
2. 在 4.26 编辑器中打开 `UMatHelperMgn` DataAsset
3. 在 `NodeButtonInfo` 数组添加条目:
   - `ButtonName`: "YourNode"（与 txt 文件名一致）
   - `RootOffsetOverride`: 是否覆盖默认偏移
   - `RootOffset`: 节点创建位置偏移
4. 点击 `Refresh HelpersButton` 刷新面板

### 添加新的菜单项

在 `MatHelper.cpp` 的 `RegisterButton()` 中添加:

```cpp
Section.AddEntry(FToolMenuEntry::InitMenuEntry(
    "YourFeature",
    LOCTEXT("YourFeature", "Your Feature"),
    LOCTEXT("YourFeature", "Description"),
    FSlateIcon(FEditorStyle::GetStyleSetName(), "YourIcon"),
    FUIAction(FExecuteAction::CreateRaw(this, &FMatHelperModule::YourMethod))
));
```

### 添加新的 AssetTypeActions

1. 创建 `FCusAssetTypeActions_YourType : public FAssetTypeActions_Base`
2. 实现 `GetSupportedClass()`、`GetTypeColor()`、`OpenAssetEditor()`、`GetThumbnailInfo()`
3. 在 `MatHelper.h` 添加 `TSharedPtr<FCusAssetTypeActions_YourType>` 成员
4. 在 `InitMatEditorHook()` 注册:
   ```cpp
   YourAssetTypeActions = MakeShareable(new FCusAssetTypeActions_YourType());
   AssetTools.RegisterAssetTypeActions(YourAssetTypeActions.ToSharedRef());
   ```
5. 在 `ShutdownModule()` 注销

### 扩展材质编辑器助手面板

在 `MatHelperWidget.cpp` 的 `Construct()` 中添加按钮:

```cpp
NodeButtonScrollBox->AddSlot()
.Padding(3.0f)
[
    SNew(SButton)
    .Text(FText::FromString("Your Button"))
    .OnClicked_Raw(this, &SMatHelperWidget::YourMethod)
];
```

### 访问引擎私有成员

**推荐方案**: `#define` hack

```cpp
#define private public
#define protected public
#include "TargetHeader.h"
#undef private
#undef protected
```

**注意事项**:
- `#define` 只对在它之后 include 的头文件生效
- PCH 已处理的头文件不受影响
- 对于 PCH 中的类，用派生类 + `reinterpret_cast`

---

## 附录：原始插件版本历史

| 版本 | 日期 | 主要内容 |
|---|---|---|
| - | 2024.3.16 | 创建材质实例、修复节点、编辑组文本 |
| - | 2024.3.20 | FixBug |
| - | 2024.3.22 | 使用 DataAsset 定义配置 |
| V4.0 | 2024.3.24 | 扩展 Palette Tool |
| V4.4 | 2024.3.29 | Niagara 播放按钮、Tag 检测 |
| V5.0 | - | 资产 Lock/UnLock、语言切换、重启 |
| V5.1 | - | 剪贴板保护、自动跳转新节点 |
| V5.1.1 | - | 修复资产不显示 |
| V6.0 | - | 自定义 Mask Pin |
| V6.1 | - | 材质实例参数批量开关 |
| V6.2 | - | 场景预览窗口 |
| **4.26** | **2026.8** | **UE 4.26 移植版** |

---

*文档最后更新: 2026-08-10*
