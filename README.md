# MatHelper (UE 4.26 移植版)

基于 [AKaKLya/MatHelper](https://github.com/AKaKLya/MatHelper) (UE 5.3) 移植到 Unreal Engine 4.26。

## 功能

- 材质编辑器助手面板 + 自定义节点按钮（Config/AddNodeFile/*.txt 模板）
- 资产图标颜色区分（材质红 / 实例绿）
- Niagara 播放按钮 + Tag 检测机制
- 资产 Lock/UnLock（禁止导出）
- SceneView 场景预览窗口（9 个 + 材质/实例/Niagara 编辑器内）
- 中英语言切换 + 重启引擎
- 常用地图快速打开下拉
- 世界大纲 Niagara Lock 列
- 材质实例编辑器参数批量开关（Enable/Disable Params）

## UE 4.26 移植说明

### 架构变更
- `UAssetDefinition` 体系 → `FAssetTypeActions` 体系（3 个类重写）
- `FAppStyle` → `FEditorStyle`（全局替换）
- `UDeveloperSettingsBackedByCVars` → `UDeveloperSettings`
- 菜单路径 `AssetEditor.MaterialEditor.ToolBar` → `AssetEditor.MaterialEditorApp.ToolBar`
- `AssetViewUtils::OpenEditorForAsset` → `UAssetEditorSubsystem::OpenEditorForAsset`
- `UEditorAssetSubsystem` → `LoadObject` + `DuplicateObject` + `SavePackage`
- `UEditorActorSubsystem::GetSelectedLevelActors` → `GEditor->GetSelectedActors()`

### 删除的 UE5 专属功能
- Detail Customization（死代码，未实际注册——参数开关由 SMatInstanceHelper 独立实现）
- Niagara Sequencer 自动轨道（`ILevelSequenceModule::OnNewActorTrackAdded` 在 4.26 不存在）
- SVG Slate brush（4.26 不支持 `FSlateVectorImageBrush`）
- `FIntVector4` USTRUCT（4.26 无 USTRUCT 标记，改用 `FLinearColor`）
- C++17 `inline static`（4.26 默认 C++14）

### 私有成员访问
4.26 的 MSVC 对模板黑魔法访问 private/protected 成员更严格。改用：
- `#define private public` / `#define protected public` 包裹引擎头文件 include
- `FCompoundWidgetAccessor` 派生类暴露 `SCompoundWidget::ChildSlot`

## 构建

1. 将本插件放入 UE 4.26 项目的 `Plugins/` 目录
2. 确保 `Niagara` 插件已启用
3. 编译项目

## Content 资源

`Content/` 下的 uasset 是 UE5 格式，可能需要在 4.26 中重建：
- `MatHelper.uasset`（DataAsset 配置）
- `Material/MI_Empty.uasset`（空材质实例模板）
- `Material/M_SlateIcon.uasset`（图标材质）

## 原作者

- AKaKLya — [GitHub](https://github.com/AKaKLya/MatHelper)
- 虚幻商城搜索 "MatHelper"

## License

遵循原作者的 license。
