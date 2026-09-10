# 《城邦争霸》战术小地图实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为现有 UE 5.8 单人遭遇战增加支持摄像机点击/拖动、右键智能命令、战争迷雾和两种方向模式的战术小地图。

**Architecture:** 新建纯 C++ UMG `UStrategyMinimapWidget`，用 `FStrategyMinimapProjection` 统一世界坐标、绘制坐标和输入坐标。控件读取现有 GameState、FogOfWar 和地图定义，复用 PlayerController 的命令与 Pawn 的摄像机边界，不创建 `SceneCapture2D`、第二套迷雾或第二套单位状态。

**Tech Stack:** Unreal Engine 5.8、C++、UMG、Slate 绘制、现有 Automation Tests、PowerShell、AutomationTool。

**Spec:** `docs/superpowers/specs/2026-09-09-city-state-rts-minimap-design.md`

## Global Constraints

- 所有文件和文本使用 UTF-8；代码标识符使用英文，注释使用中文。
- 小地图默认固定北向，允许在设置中切换为跟随摄像机并持久保存。
- 小地图范围使用 `FogMin=(-16000,-14000)`、`FogMax=(16000,14000)`，不扩大摄像机合法范围。
- 己方对象始终显示；敌方对象仅当前可见时显示；已探索城镇保留图标但隐藏未获知详情。
- 左键点击或拖动移动摄像机；右键可见敌军为攻击，其余地面为移动。
- 小地图输入必须被控件消费，不能触发战场框选、世界队徽拖动或重复命令。
- 不增加小地图缩放、折叠、信号标记、单位框选、攻击移动快捷键或实时三维捕获。
- 减少测试频率：只为坐标、可见性和命令判定编写纯规则测试；中间阶段只做必要编译，最终只运行一次完整自动化、一次打包和一次启动检查。
- 保留现有注释、文档注释和未完成标记；不改动无关玩法。
- 当前工作区包含大量用户改动，不提交、不重置、不覆盖无关文件。

## File Structure

- Create: `Source/RTS/Variant_Strategy/UI/StrategyMinimapModel.h` — 小地图方向枚举、投影、可见性、命令意图和响应式尺寸纯规则。
- Create: `Source/RTS/Variant_Strategy/UI/StrategyMinimapModel.cpp` — 纯规则实现。
- Create: `Source/RTS/Variant_Strategy/UI/StrategyMinimapWidget.h` — UMG 控件接口、输入状态和图标快照。
- Create: `Source/RTS/Variant_Strategy/UI/StrategyMinimapWidget.cpp` — Slate 绘制、动态数据收集和鼠标交互。
- Modify: `Source/RTS/Variant_Strategy/StrategyMapDefinition.h` — 小地图水域和道路矩形数据。
- Modify: `Source/RTS/Variant_Strategy/StrategyMapDefinition.cpp` — 河谷地图战术底图数据。
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h` — 迷雾探索状态和玩家迷雾纹理只读接口。
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp` — 迷雾只读接口实现。
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.h` — 面向 UI 的探索状态与迷雾纹理代理。
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.cpp` — GameState 迷雾代理实现。
- Modify: `Source/RTS/Variant_Strategy/StrategyGameUserSettings.h` — 小地图方向配置与设置草稿。
- Modify: `Source/RTS/Variant_Strategy/StrategyGameUserSettings.cpp` — 默认值、草稿、应用及保存。
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyPauseMenu.h` — 小地图方向下拉框与回调。
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyPauseMenu.cpp` — 设置菜单控件和即时预览。
- Modify: `Source/RTS/Variant_Strategy/StrategyPawn.h` — 按世界地面焦点定位摄像机。
- Modify: `Source/RTS/Variant_Strategy/StrategyPawn.cpp` — 摄像机焦点换算与边界限制。
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.h` — 小地图摄像机、命令和视野四角入口。
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp` — 复用现有命令与摄像机逻辑。
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.h` — 保存小地图控件引用。
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.cpp` — 创建、布局和响应式缩放小地图。
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyHUDTests.cpp` — 小地图纯规则自动化测试。
- Modify: `docs/ROADMAP.md` — 登记小地图阶段状态。
- Create: `docs/M7-CURRENT.md` — 当前候选、操作说明、验证结果与人工验收项。
- Create: `docs/verification/M7-minimap-tests.json` — 最终自动化摘要。
- Create: `docs/verification/M7-minimap-smoke.json` — 启动检查摘要。
- Create: `docs/verification/M7-minimap-package.csv` — 打包文件校验清单。

---

### Task 1: 小地图投影、可见性与命令纯规则

**Files:**
- Create: `Source/RTS/Variant_Strategy/UI/StrategyMinimapModel.h`
- Create: `Source/RTS/Variant_Strategy/UI/StrategyMinimapModel.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyHUDTests.cpp`

**Interfaces:**
- Consumes: `EStrategyFaction`、世界地图矩形和摄像机 yaw。
- Produces: `EStrategyMinimapOrientation`、`FStrategyMinimapProjection::WorldToLocal`、`LocalToWorld`、`FStrategyMinimapVisibilityRules::ShouldDraw`、`FStrategyMinimapCommandRules::ResolveIntent`、`FStrategyMinimapLayoutRules::ResolveSize`。

- [x] **Step 1: 写投影失败测试**

在 `StrategyHUDTests.cpp` 引入尚未存在的 `StrategyMinimapModel.h`，注册 `RTS.Strategy.UI.MinimapProjection`。测试固定北向的四角、中心和往返误差，并验证跟随摄像机模式使用相同逆变换：

```cpp
const FVector2D MapMin(-16000.0f, -14000.0f);
const FVector2D MapMax(16000.0f, 14000.0f);
const FVector2D Size(300.0f, 240.0f);
TestTrue(TEXT("北西角映射到左上"), FStrategyMinimapProjection::WorldToLocal(
	FVector2D(-16000.0f, 14000.0f), MapMin, MapMax, Size,
	EStrategyMinimapOrientation::NorthUp, -45.0f).Equals(FVector2D(0.0f, 0.0f), 0.01f));
const FVector2D World(4200.0f, -3100.0f);
const FVector2D FollowLocal = FStrategyMinimapProjection::WorldToLocal(
	World, MapMin, MapMax, Size, EStrategyMinimapOrientation::FollowCamera, -45.0f);
TestTrue(TEXT("旋转模式必须可逆"), FStrategyMinimapProjection::LocalToWorld(
	FollowLocal, MapMin, MapMax, Size, EStrategyMinimapOrientation::FollowCamera, -45.0f).Equals(World, 1.0f));
```

- [x] **Step 2: 构建并确认 RED**

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\Build.bat' RTSEditor Win64 Development 'D:\ue project\RTS\RTS.uproject' -WaitMutex -NoHotReloadFromIDE -utf8output
```

Expected: 编译因 `StrategyMinimapModel.h` 或声明不存在而失败，证明测试确实覆盖新接口。

- [x] **Step 3: 实现最小投影规则**

在新模型头文件中定义：

```cpp
UENUM()
enum class EStrategyMinimapOrientation : uint8
{
	NorthUp,
	FollowCamera
};

enum class EStrategyMinimapEntityKind : uint8
{
	Squad,
	Building,
	Capital,
	Town
};

enum class EStrategyMinimapCommandIntent : uint8
{
	None,
	Move,
	Attack
};

struct FStrategyMinimapProjection
{
	static FVector2D WorldToLocal(const FVector2D& World, const FVector2D& MapMin,
		const FVector2D& MapMax, const FVector2D& LocalSize,
		EStrategyMinimapOrientation Orientation, float CameraYaw);
	static FVector2D LocalToWorld(const FVector2D& Local, const FVector2D& MapMin,
		const FVector2D& MapMax, const FVector2D& LocalSize,
		EStrategyMinimapOrientation Orientation, float CameraYaw);
};
```

实现以地图中心为原点。固定北向时 `+X` 向右、`+Y` 向上；跟随摄像机时先按 `90° - CameraYaw` 旋转世界偏移，使摄像机前方向上。旋转后用 `abs(cos)*HalfX + abs(sin)*HalfY` 和 `abs(sin)*HalfX + abs(cos)*HalfY` 计算完整地图的包围半径，再映射到控件，逆变换严格执行相反顺序。

- [x] **Step 4: 补齐可见性、命令与布局测试**

在同一测试文件注册 `RTS.Strategy.UI.MinimapRules`，覆盖：

```cpp
TestTrue(TEXT("己方对象始终显示"), FStrategyMinimapVisibilityRules::ShouldDraw(
	EStrategyMinimapEntityKind::Squad, EStrategyFaction::Player, false, false));
TestFalse(TEXT("不可见敌军不能显示"), FStrategyMinimapVisibilityRules::ShouldDraw(
	EStrategyMinimapEntityKind::Squad, EStrategyFaction::Enemy, false, true));
TestTrue(TEXT("当前可见敌军可以显示"), FStrategyMinimapVisibilityRules::ShouldDraw(
	EStrategyMinimapEntityKind::Building, EStrategyFaction::Enemy, true, true));
TestTrue(TEXT("已探索城镇保留图标"), FStrategyMinimapVisibilityRules::ShouldDraw(
	EStrategyMinimapEntityKind::Town, EStrategyFaction::Neutral, false, true));
TestTrue(TEXT("固定中立城镇允许显示无归属图标"), FStrategyMinimapVisibilityRules::ShouldDraw(
	EStrategyMinimapEntityKind::Town, EStrategyFaction::Neutral, false, false));
TestEqual(TEXT("无选择不下令"), FStrategyMinimapCommandRules::ResolveIntent(false, true),
	EStrategyMinimapCommandIntent::None);
TestEqual(TEXT("可见敌军触发攻击"), FStrategyMinimapCommandRules::ResolveIntent(true, true),
	EStrategyMinimapCommandIntent::Attack);
TestEqual(TEXT("其余位置触发移动"), FStrategyMinimapCommandRules::ResolveIntent(true, false),
	EStrategyMinimapCommandIntent::Move);
TestTrue(TEXT("720p 小地图按比例缩小"),
	FStrategyMinimapLayoutRules::ResolveSize(720.0f).Equals(FVector2D(250.0f, 200.0f)));
TestTrue(TEXT("1080p 使用默认尺寸"),
	FStrategyMinimapLayoutRules::ResolveSize(1080.0f).Equals(FVector2D(300.0f, 240.0f)));
```

实现 `ShouldDraw`、`ResolveIntent` 和 `ResolveSize`。布局规则仅使用 `720p -> 250×200`、`1080p 及以上 -> 300×240` 的线性限制，不引入通用响应式框架。

- [x] **Step 5: 构建并只运行两个小地图规则测试**

Run editor build, then:

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests RTS.Strategy.UI.Minimap' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/ue project/RTS/Saved/Verification/M7/MinimapRules' '-abslog=D:/ue project/RTS/Saved/Verification/M7/MinimapRules.log' -nosplash
```

Expected: `MinimapProjection` 与 `MinimapRules` 成功，0 failure。此后不重复运行，直至 Task 6 最终验证。

### Task 2: 开放只读迷雾数据并描述战术底图

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyMapDefinition.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyMapDefinition.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.cpp`

**Interfaces:**
- Consumes: `FStrategyFogGrid`、`FogTexture`、`FStrategySkirmishMapDefinition`。
- Produces: `AStrategyGameState::IsExploredToFaction`、`GetPlayerFogTexture`、`MinimapWaterAreas`、`MinimapRouteAreas`。

- [x] **Step 1: 增加迷雾只读接口**

向 `AStrategyFogOfWar` 增加：

```cpp
bool IsExploredToFaction(EStrategyFaction Faction, const FVector& Location) const;
UTexture2D* GetPlayerFogTexture() const { return FogTexture; }
```

实现与 `IsVisibleToFaction` 相同的阵营网格选择，只调用 `FStrategyFogGrid::IsExplored`，不修改迷雾更新周期和纹理内容。

- [x] **Step 2: 通过 GameState 暴露 UI 代理**

在 `AStrategyGameState` 增加：

```cpp
bool IsExploredToFaction(EStrategyFaction Faction, const FVector& Location) const;
UTexture2D* GetPlayerFogTexture() const;
```

两者直接转发给 `FogOfWar`。现有 `IsVisibleToFaction` 行为保持不变。

- [x] **Step 3: 为地图定义增加战术区域**

向 `FStrategySkirmishMapDefinition` 增加：

```cpp
TArray<FBox2D> MinimapWaterAreas;
TArray<FBox2D> MinimapRouteAreas;
```

河谷地图写入与 `InitializeRiverValley` 一致的四段河流，以及南桥、北桥和中央浅滩通路矩形。基础遭遇战保持空数组，只绘制基础地面和地图轮廓。该数据仅负责战术显示，不改变碰撞、导航或建造限制。

- [x] **Step 4: 编译检查接口集成**

运行一次 RTSEditor Development 构建。Expected: 编译成功；不运行自动化，不启动编辑器。

### Task 3: 小地图方向设置与菜单

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyGameUserSettings.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameUserSettings.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyPauseMenu.h`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyPauseMenu.cpp`

**Interfaces:**
- Consumes: Task 1 的 `EStrategyMinimapOrientation`。
- Produces: `UStrategyGameUserSettings::GetMinimapOrientation`、`SetMinimapOrientation`、草稿字段 `MinimapOrientation`、设置页“小地图方向”下拉框。

- [x] **Step 1: 扩展持久化设置与草稿**

在设置头文件中包含 `StrategyMinimapModel.h`，并增加：

```cpp
struct FStrategySettingsDraft
{
	// 保留现有字段
	EStrategyMinimapOrientation MinimapOrientation = EStrategyMinimapOrientation::NorthUp;
};

EStrategyMinimapOrientation GetMinimapOrientation() const { return MinimapOrientation; }
void SetMinimapOrientation(EStrategyMinimapOrientation Value) { MinimapOrientation = Value; }

UPROPERTY(Config)
EStrategyMinimapOrientation MinimapOrientation = EStrategyMinimapOrientation::NorthUp;
```

`SetToDefaults` 设置 `NorthUp`，`MakeDraft` 复制配置，`ApplyDraft` 在 `ApplySettings(false)` 前写入草稿值，随后沿用现有 `SaveSettings()`。`SetMinimapOrientation` 只更新当前运行值，用于设置页即时预览，不自行保存。

- [x] **Step 2: 在设置页增加方向下拉框**

在暂停菜单保存 `UComboBoxString* MinimapOrientationCombo`，设置页增加“小地图方向”一行，选项顺序固定为：

```cpp
MinimapOrientationCombo->AddOption(TEXT("固定北向"));
MinimapOrientationCombo->AddOption(TEXT("跟随摄像机"));
```

`RefreshSettingsControls` 用枚举值设置索引。打开设置页时把当前值另存为 `OriginalMinimapOrientation`。`HandleMinimapOrientationChanged` 在非 `bUpdatingControls` 状态下同时修改 `Draft.MinimapOrientation` 并调用 `SetMinimapOrientation`，小地图下一帧读取新值，实现即时预览。“应用并返回”由 `ApplyDraft` 持久保存；“放弃并返回”与从设置页按 Esc 都调用 `SetMinimapOrientation(OriginalMinimapOrientation)`，恢复进入设置页之前的值。

- [x] **Step 3: 增加设置草稿断言**

在现有 `RTS.Strategy.Systems.SettingsRules` 中增加无引擎启动依赖的枚举索引断言，并把索引转换集中到：

```cpp
static int32 MinimapOrientationToIndex(EStrategyMinimapOrientation Value);
static EStrategyMinimapOrientation MinimapOrientationFromIndex(int32 Index);
```

断言 `NorthUp <-> 0`、`FollowCamera <-> 1`。不单独运行测试，留到 Task 6 一次性验证。

- [x] **Step 4: 编译检查设置与 UHT**

运行一次 RTSEditor Development 构建。Expected: UHT、设置配置属性和菜单动态回调全部编译成功。

### Task 4: 摄像机定位、视野框和右键命令入口

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyPawn.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyPawn.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`

**Interfaces:**
- Consumes: Task 1 的 `EStrategyMinimapCommandIntent`，现有 `DoIssueOrder`、`FStrategyOrderTargetRules` 和摄像机边界规则。
- Produces: `AStrategyPawn::SetGroundFocus`、`AStrategyPlayerController::MoveCameraFromMinimap`、`IssueMinimapCommand`、`GetCameraGroundCorners`。

- [x] **Step 1: 提取 Pawn 地面焦点定位**

增加：

```cpp
void SetGroundFocus(const FVector2D& DesiredFocus, float ViewportAspectRatio);
```

实现沿当前摄像机 forward vector 计算现有地面焦点，移动 Pawn 使焦点落到 `DesiredFocus`，随后调用 `ClampToMapBounds(ViewportAspectRatio)`。不改变 Pawn 高度、yaw、pitch 或缩放。

- [x] **Step 2: 增加 PlayerController 摄像机入口**

增加：

```cpp
void MoveCameraFromMinimap(const FVector2D& WorldLocation);
bool GetCameraGroundCorners(TArray<FVector2D>& OutCorners) const;
```

`MoveCameraFromMinimap` 从 viewport 宽高计算 aspect ratio，并调用 Pawn 的 `SetGroundFocus`。`GetCameraGroundCorners` 对 `(0,0)`、`(Width,0)`、`(Width,Height)`、`(0,Height)` 调用 `DeprojectScreenPositionToWorld`，再与 `Z=0` 平面求交，按屏幕顺序返回四个世界 XY，用于小地图视野框。

- [x] **Step 3: 增加复用现有规则的智能命令入口**

增加：

```cpp
void IssueMinimapCommand(const FVector2D& WorldLocation, AActor* VisibleEnemyTarget);
```

方法先调用 `FStrategyMinimapCommandRules::ResolveIntent(!ControlledSquads.IsEmpty(), IsValid(VisibleEnemyTarget))`。返回 `Attack` 时建立 `AttackTarget` 命令并使用目标当前位置；返回 `Move` 时建立地面 `Move` 命令；返回 `None` 时直接结束。最终统一调用 `DoIssueOrder`，从而保留驻防出击、编队、反馈和目标处理。

控件只能传入从“当前绘制的敌军图标”命中的 Actor，因此这里不再进行第二次屏幕命中测试，也不改变主视图右键逻辑。

- [x] **Step 4: 编译检查控制器接口**

运行一次 RTSEditor Development 构建。Expected: 新接口与现有命令、摄像机类型匹配；不运行自动化。

### Task 5: UMG 小地图绘制、命中测试与 HUD 集成

**Files:**
- Create: `Source/RTS/Variant_Strategy/UI/StrategyMinimapWidget.h`
- Create: `Source/RTS/Variant_Strategy/UI/StrategyMinimapWidget.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.h`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.cpp`

**Interfaces:**
- Consumes: Tasks 1–4 的投影、设置、迷雾、GameState 集合、摄像机和命令入口。
- Produces: 可绘制并可交互的 `UStrategyMinimapWidget`，由 `UStrategyHUDRoot` 持有。

- [x] **Step 1: 声明控件和输入生命周期**

`StrategyMinimapWidget.h` 声明：

```cpp
UCLASS()
class UStrategyMinimapWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void InitializeForController(AStrategyPlayerController* InController);
protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
		const FSlateRect& CullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;
private:
	void MoveCameraAtPointer(const FGeometry& Geometry, const FPointerEvent& Event);
	AActor* FindVisibleEnemyAt(const FVector2D& LocalPosition, const FGeometry& Geometry) const;
	bool bDraggingCamera = false;
	UPROPERTY(Transient) TObjectPtr<AStrategyPlayerController> Controller;
	mutable FSlateBrush FogBrush;
	TMap<TWeakObjectPtr<AStrategyControlPoint>, EStrategyFaction> ObservedTownOwners;
};
```

左键按下返回 `FReply::Handled().CaptureMouse(TakeWidget())` 并定位摄像机；拖动期间持续定位；左键释放返回 `ReleaseMouseCapture()`；右键调用 `IssueMinimapCommand`。鼠标离开、控件失焦或暂停时把 `bDraggingCamera` 清零。

- [x] **Step 2: 收集统一图标快照**

在 cpp 内定义只在该文件使用的紧凑结构：

```cpp
struct FStrategyMinimapIcon
{
	TWeakObjectPtr<AActor> Actor;
	FVector2D LocalPosition = FVector2D::ZeroVector;
	FLinearColor Color = FLinearColor::White;
	float Radius = 4.0f;
	bool bEnemyTarget = false;
	bool bGarrisoned = false;
};
```

遍历 `GetControlPoints()`、`GetBuildings()` 和 `GetSquads()`。每个对象统一读取 `IsVisibleToFaction(Player)` 与 `IsExploredToFaction(Player)`，再调用 `FStrategyMinimapVisibilityRules::ShouldDraw`。敌军图标只有当前可见时进入数组；`bEnemyTarget` 仅对实现 `IStrategyDamageable` 且存活的敌军设置。驻防小队使用 `GetGarrisonPoint()` 的位置加按索引排列的小偏移，并绘制空心驻防环。

中立城镇位置属于固定地图信息，未探索时也可绘制无归属色图标。城镇当前可见或属于玩家时更新 `ObservedTownOwners`；城镇进入已探索但不可见状态后，只使用缓存的最后已知归属着色，不能直接读取并显示视野外发生的归属变化。

- [x] **Step 3: 绘制缓存底图和迷雾四边形**

使用 `FSlateDrawElement` 分层绘制：

1. 深色边框和绿色基础地图四边形。
2. `MinimapWaterAreas` 蓝色矩形和 `MinimapRouteAreas` 棕色矩形。
3. 玩家迷雾纹理。
4. 据点、建筑和小队图标。
5. 摄像机视野四边形、`N` 标记和边框。

底图几何只在地图名、控件尺寸或方向变化时重建；图标与摄像机框每帧重建。迷雾使用 `FogBrush.SetResourceObject(State->GetPlayerFogTexture())`，四个顶点分别使用世界地图四角经过 `WorldToLocal` 的位置和 `(0,0)`、`(1,0)`、`(1,1)`、`(0,1)` UV。由于纹理第 0 行对应 `FogMin.Y`，UV 按世界角点绑定，不按屏幕上下硬编码，从而在两种方向模式下保持一致。

- [x] **Step 4: 实现图标命中与右键请求**

右键位置转换为控件局部坐标，按图标绘制逆序查找 `bEnemyTarget`，使用 `max(8 px, Radius + 3 px)` 的圆形命中半径。命中后把 Actor 传给 `IssueMinimapCommand`；未命中时把局部坐标经 `LocalToWorld` 转为地面位置并传空目标。

只有控件矩形内的事件进入这条链路。所有已处理的左右键事件返回 `Handled`，防止 Enhanced Input 同帧触发战场选择或互动。

- [x] **Step 5: 接入 HUDRoot 布局**

在 `RebuildWidget` 中创建 `UStrategyMinimapWidget` 和外层 `USizeBox`。Canvas 布局使用右下锚点、右对齐和底部对齐，默认偏移为距右侧 `20 px`、距底部操作栏顶部 `16 px`。调用 `InitializeForController(Controller)`。

`ApplyViewportScale` 使用 `FStrategyMinimapLayoutRules::ResolveSize(Height)` 设置 SizeBox 的 width/height，不对小地图施加小数 RenderScale；这样避免已经修复过的 UI 文字和图标抖动。城镇面板显示时小地图位置不变，两者的区域不得相交。

- [ ] **Step 6: 编译并做一次编辑器交互检查**

构建 RTSEditor Development，打开 `LVL_CityStateSkirmish` PIE，仅检查：小地图可见、左拖摄像机、右键地面、右键可见敌军、暂停继续后左右键仍有效。发现编译或输入问题时修正后只重做这一短检查，不运行完整测试。

### Task 6: 集中验证、打包和阶段记录

**Files:**
- Modify: `docs/ROADMAP.md`
- Create: `docs/M7-CURRENT.md`
- Create: `docs/verification/M7-minimap-tests.json`
- Create: `docs/verification/M7-minimap-smoke.json`
- Create: `docs/verification/M7-minimap-package.csv`
- Create: `Builds/Windows_M7_Minimap/`

**Interfaces:**
- Consumes: Tasks 1–5 的完整小地图实现。
- Produces: 自动化结果、Windows Development 候选、启动证据、校验清单和人工验收说明。

- [x] **Step 1: 最终编辑器构建**

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\Build.bat' RTSEditor Win64 Development 'D:\ue project\RTS\RTS.uproject' -WaitMutex -NoHotReloadFromIDE -utf8output
```

Expected: `Build succeeded`，无 UHT、编译或链接错误。

- [x] **Step 2: 一次性运行完整自动化组**

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests RTS.Strategy' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/ue project/RTS/Saved/Verification/M7/Automation' '-abslog=D:/ue project/RTS/Saved/Verification/M7/Automation.log' -nosplash
```

Expected: 全部 `RTS.Strategy` 测试成功，0 warning、0 failure。把实际测试数、结果和日志关键字统计写入 `M7-minimap-tests.json`。

- [ ] **Step 3: 人工检查布局、方向和信息边界**

依次检查 `1280×720`、`1920×1080`、`1920×1200` 和当前超宽屏：

- 小地图不覆盖底部栏和城镇管理面板。
- 固定北向默认生效并显示 `N`。
- 跟随摄像机时底图、迷雾、图标、视野框和点击位置同步旋转。
- 己方对象始终显示；敌军离开当前视野后立即消失；已探索城镇保留图标。
- 左键点击与拖动摄像机都受原边界限制。
- 右键地面移动，右键可见敌军攻击，无已选小队时不下令。
- 驻防小队在城镇附近显示驻防标记。
- 按 Esc、进入设置、放弃或应用、继续游戏后左右键和滚轮都正常。

只记录实际观察结果，不把自动化测试代替人工输入验证。

- [x] **Step 4: 构建一次 Windows 候选**

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun '-project=D:/ue project/RTS/RTS.uproject' -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive '-archivedirectory=D:/ue project/RTS/Builds/Windows_M7_Minimap' -utf8output
```

Expected: `BUILD SUCCESSFUL`，最终程序位于 `D:\ue project\RTS\Builds\Windows_M7_Minimap\RTS.exe`。

- [x] **Step 5: 一次启动冒烟与文件校验**

启动打包程序 20 秒，确认进程保持运行；扫描独立日志中的 `Fatal error`、`Assertion failed`、`Unhandled Exception` 和项目资源加载失败。结束进程后清理包内运行日志，生成包内每个文件的相对路径、字节数和 SHA-256 到 `M7-minimap-package.csv`，并复核 0 不匹配。把启动时长、退出方式和关键字计数写入 `M7-minimap-smoke.json`。

- [x] **Step 6: 维护最近五个候选并更新文档**

按最后写入时间列出 `D:\ue project\RTS\Builds\Windows_*`。若超过五个，只对超出的最旧目录执行清理，并在删除前逐一确认解析后的绝对路径直接位于 `D:\ue project\RTS\Builds` 下。`docs/M7-CURRENT.md` 写明运行路径、操作方式、设置入口、自动化结果、冒烟结果和人工验收清单；`docs/ROADMAP.md` 将小地图标记为待玩家最终验收。

## Review Checkpoints

- Task 1 后审查投影正反变换、方向角和规则测试，不允许显示层自行重算坐标。
- Task 3 后审查设置草稿的应用/放弃语义，确保放弃不会持久化预览值。
- Task 4 后审查小地图只调用现有 `DoIssueOrder`，不复制驻防出击和命令反馈。
- Task 5 后重点审查输入消费、鼠标捕获释放、迷雾 UV 和超宽屏布局。
- Task 6 后以人工操作和最终包为准完成验收，不因自动化通过直接宣称交互无误。
