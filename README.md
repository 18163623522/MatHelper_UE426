# MatHelper UE 4.26 移植版

基于 [AKaKLya/MatHelper](https://github.com/AKaKLya/MatHelper)（UE 5.3）移植到 **Unreal Engine 4.26** 的材质编辑器辅助插件，界面已全面中文化。

- 移植仓库：https://github.com/18163623522/MatHelper_UE426
- 技术细节：见 [DEVELOPMENT.md](DEVELOPMENT.md)（架构、UE5→4.26 API 映射表、移植决策记录）
- 已在 UE 4.26 实际项目中编译通过并完成运行时修复验证

---

## 功能清单

### ✅ 可用功能

| 功能 | 入口 |
|---|---|
| 材质编辑器助手面板 | 打开任意材质，Palette 面板上方 |
| ├ MatHelper 管理器 | 打开配置 DataAsset |
| ├ 场景视图 | 编辑器内嵌视口 |
| ├ 设置分组 / 自动分组 / 全部自动分组 | 参数节点分组 |
| ├ 添加 Mask 引脚（R/G/B/A/RGB/RGBA/RG/BA） | 下拉选择 + 按钮 |
| ├ 显示引脚名称 | 切换 Pin 名 |
| ├ 创建实例 | 从当前材质生成 MIC 并全开参数 |
| ├ 折射 | 切换折射模式 |
| ├ 修复函数节点 / 自动命名 | 节点维护 |
| └ 节点按钮（Fresnel / ParticleColor） | 从 txt 模板一键创建节点 |
| 材质实例参数批量开关 | 材质实例编辑器工具栏「开启所有参数 / 关闭所有参数」 |
| 资产图标颜色区分 | 材质红色、材质实例绿色（内容浏览器） |
| 资产锁定 / 解锁 | 工具 → 材质助手（禁止右键导出） |
| 场景视图窗口 ×12 | 工具 → 材质助手 → 场景视图（可在任意编辑器中打开） |
| 常用地图快速打开 | 主编辑器工具栏下拉 |
| 世界大纲 Niagara 锁定列 | 大纲复选框，勾选的 Actor 添加 `NiagaraAutoPlay` Tag |
| Niagara 播放 | 主工具栏 / 材质 / 材质实例 / Niagara 编辑器按钮，播放带 Tag 的特效 |
| 中英语言切换 / 重启引擎 | 工具 → 材质助手 |

### ❌ 完全缺失（4.26 引擎硬限制，无法恢复）

| 功能 | 原版行为 | 缺失原因 |
|---|---|---|
| Niagara Sequencer 自动轨道 | Niagara Actor 拖入定序器时，自动创建 "System Life Cycle" 轨道并设为 DesiredAge 模式 | 4.26 的 `ILevelSequenceModule` 只有 `RegisterObjectSpawner`，没有 `OnNewActorTrackAdded` 委托，无可挂接入口。`AddDefaultSystemTracks` 整段移除，DataAsset 里的 `OverrideNiagaraSequenceMode` 配置项保留但无效 |

### ⚠️ 功能降级（行为改变，非完全丢失）

| 功能 | 原版行为 | 4.26 现状 |
|---|---|---|
| 折射按钮 | 在 `RM_None`（关闭）与 `RM_IndexOfRefraction` 之间切换 | 4.26 枚举没有 `RM_None`，改为在 `RM_IndexOfRefraction` / `RM_PixelNormalOffset` 两种模式间切换，无法"关闭"折射 |
| SVG 按钮图标 | Niagara 播放等按钮用 SVG 矢量图标 | 4.26 无 `FSlateVectorImageBrush`，按钮纯色显示；`ModifyICON` 中基于 SVG 的分支退化为空 brush（基于材质的分支保留） |
| CheckNode 节点过滤 | 排除 Composite / PinBase 等节点类型 | 4.26 引擎本身没有 `UMaterialGraphNode_Composite` / `PinBase` 类，相应检查移除（无实际影响） |

### 📦 配置数据缺失（重建 DataAsset 即可恢复）

原版 DataAsset（UE5 格式，4.26 加载不了）中的**自定义节点按钮列表、自动分组关键词（`AutoGroupKeys`）**等数据未随插件迁移。当前内置兜底仅有：

- 节点按钮：Fresnel、ParticleColor 两个（对应 `Config/AddNodeFile/` 现成模板）
- 自动分组关键词：空（「自动分组」无关键词可用时相当于不分组；「设置分组」手动输入不受影响）

在 4.26 编辑器重建 `UMatHelperMgn` DataAsset 补齐这些数组即可完整恢复。

### ℹ️ 疑似缺失、核实后不算

| 项目 | 核实结论 |
|---|---|
| 材质实例 Detail Customization（1700+ 行 × 3 个版本文件） | 原版从未注册的**死代码**，不生效，移植版删除无影响 |
| Material Layers 图层树（`SMaterialLayersFunctionsInstanceTree`） | 原版中主体处于注释块内，非启用功能 |
| V6.1 参数批量开关 | 原版 `SMatInstanceHelper` 无实例化点（原版实际也触发不了）；移植版已挂到材质实例编辑器工具栏，**比原版更完整** |

---

## 安装

1. 将本目录复制到 UE 4.26 项目的 `Plugins/` 下（可改名 `MatHelper`）
2. 在 `.uproject` 中启用：
   ```json
   "Plugins": [
     { "Name": "MatHelper", "Enabled": true },
     { "Name": "Niagara", "Enabled": true }
   ]
   ```
3. 右键 `.uproject` → Generate Visual Studio project files → 编译（Development Editor | Win64）
4. 打开编辑器，菜单 **工具 → 材质助手**，或双击任意材质查看助手面板

### Content 资源说明

`Content/` 下的 3 个 uasset 是 UE5 序列化格式，**4.26 无法加载**（日志会报 "Package is too old"）。插件已内置兜底：

- `MatHelper.uasset`（配置）→ 加载失败时自动创建临时默认对象（红色材质/绿色实例、内置 Fresnel/ParticleColor 按钮），日志有 Warning 提示
- `MI_Empty.uasset`（实例模板）→ 「创建实例」按钮检测到加载失败时直接 `NewObject` 新建 MIC，功能等价
- `M_SlateIcon.uasset` → 仅影响图标材质，可忽略

**建议**：在 4.26 编辑器中重建 `UMatHelperMgn` DataAsset（Content Browser → Miscellaneous → Data Asset → MatHelperMgn），保存到 `/MatHelper/MatHelper` 覆盖后即可自定义节点按钮、颜色、偏移、自动分组关键词等全部配置。

### 自定义节点按钮

1. 在材质编辑器里复制目标节点，粘贴到 `Config/AddNodeFile/<名字>.txt`（清理 `ExportPath` 等路径字段）
2. 打开 MatHelper DataAsset，在 `NodeButtonInfo` 数组添加条目，`ButtonName` 与 txt 文件名一致
3. 点击 DataAsset 里的「刷新节点按钮」（或重启编辑器）

---

## ⚠️ 重要注意事项

### 不要用 Live Coding（Ctrl+Alt+F11）迭代本插件

本插件通过 `reinterpret_cast` 注入 Slate `ChildSlot` 并访问引擎私有成员。Live Coding 热重载后旧控件内存布局与新代码不一致，**必然出现乱码（如「翡器鷄胎膀掞」）或崩溃**。曾实测复现。

正确流程：**关闭编辑器 → 改代码 → UBT/VS 编译 → 打开编辑器**

### 中文显示的技术约束

4.26 的 Unity Build 把所有 cpp 合并为一个无 BOM 的文件，MSVC 按本地编码（GBK）解析，中文字面量在编译期即损坏。BOM、`#pragma execution_character_set`、`UTF8_TO_TCHAR` 均不能可靠解决，最终方案是**全部中文用 `L"\uXXXX"` Unicode 转义**（不受源文件编码影响）。新增中文文案时请遵循同样写法，转换命令：

```bash
python3 -c "print(''.join('\\\\u%04x' % ord(c) if ord(c) > 127 else c for c in '你的文字'))"
```

### 多编辑器实例

同时开两个 UE4Editor 进程测试 Slate 注入类插件会互相干扰，请单实例运行。

---

## 与原版（UE 5.3）的差异摘要

| 类别 | 变更 |
|---|---|
| 架构 | `UAssetDefinition` 体系 → `FAssetTypeActions` 体系（3 个类重写，经 `IAssetTools` 注册） |
| 样式 | `FAppStyle` → `FEditorStyle`（约 20 处）；SVG brush 降级为纯色 |
| 菜单 | 工具栏路径 `MaterialEditorApp` / `MaterialInstanceEditorApp`（4.26 带 App 后缀） |
| 面板注入 | 原版经 `OnRegisterTabSpawners` 注入 Palette；4.26 中 Palette 创建时机更晚，改为 `FTickableEditorObject` 延迟注入 |
| 参数开关 | 原版 `SMatInstanceHelper` 无实例化点（死代码）；移植版挂到材质实例编辑器工具栏 |
| 节点按钮 | DataAsset 加载失败时内置 Fresnel/ParticleColor 默认按钮 |
| C++ 标准 | C++17 → C++14 兼容（`inline static`、if-init、`TObjectPtr` 等全部改写） |
| 类型 | `FIntVector4`（4.26 无 USTRUCT 标记）→ `FLinearColor` |
| 私有成员 | 模板黑魔法（4.26 MSVC 拒绝）→ `#define private public` 包裹 include + 派生类 `FCompoundWidgetAccessor` |

## 原作者

- AKaKLya — [GitHub](https://github.com/AKaKLya/MatHelper)、虚幻商城 "MatHelper"
- 移植遵循原项目 License
