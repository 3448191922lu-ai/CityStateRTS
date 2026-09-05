# 小队头顶拖拽 HUD 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为每支己方存活小队增加固定屏幕尺寸的头顶 HUD，使玩家能拖到任意地面移动，或拖到可见敌方目标集火。

**Architecture:** `AStrategySquad` 只暴露生命和锚点数据，`AStrategyPlayerController` 统一处理投影命中、拖拽状态、选择与命令，`AStrategyHUD` 只绘制徽记、生命条和箭头。命令继续复用 `FStrategyOrder` 和现有小队移动战斗流程，不创建 `WidgetComponent`。

**Tech Stack:** Unreal Engine 5.8、C++、AHUD Canvas、Enhanced Input、Automation Tests。

**Spec:** `docs/superpowers/specs/2026-09-05-squad-drag-hud-design.md`

## Global Constraints

- 所有文件和文本使用 UTF-8，代码标识符使用英文，注释使用中文。
- 首版只支持 Windows 键鼠和玩家阵营，不增加第三方插件或图片资源。
- 徽记锚点高度为 260 cm，默认直径 34 像素，悬停、选中或拖动时直径 42 像素，拖拽阈值为 6 像素。
- 小队生命上限固定为小队生成时的初始总生命；成员死亡不会降低生命上限。
- 普通建筑放置与城墙拖拽优先于小队 HUD 输入。
- 当前目录不是 Git 仓库；每个任务使用编译和自动化测试作为可恢复检查点，不执行提交。

---

### Task 1: 纯规则命中、命令接收模式与生命计算

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Test: `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp`

**Interfaces:**
- Produces: `FStrategySquadMarkerRules::FindHoveredMarker(const TArray<FVector2D>&, const FVector2D&, float) -> int32`
- Produces: `FStrategySquadMarkerRules::ShouldCommandSelectedSquads(bool) -> bool`
- Produces: `FStrategySquadMarkerRules::ResolveOrderType(bool) -> EStrategyOrderType`
- Produces: `FStrategySquadMarkerRules::CalculateHealthPercent(float, float) -> float`
- Produces: `FStrategySquadMarkerRules::CanInteract(bool, bool) -> bool`

- [ ] **Step 1: 写徽记命中失败测试**

在 `StrategySystemsTests.cpp` 增加：

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategySquadMarkerRulesTest,
	"RTS.Strategy.Systems.SquadMarkerRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategySquadMarkerRulesTest::RunTest(const FString& Parameters)
{
	const TArray<FVector2D> Markers = {FVector2D(100.0f, 100.0f), FVector2D(120.0f, 100.0f)};
	TestEqual(TEXT("重叠命中时返回离光标最近的徽记"),
		FStrategySquadMarkerRules::FindHoveredMarker(Markers, FVector2D(116.0f, 100.0f), 17.0f), 1);
	TestEqual(TEXT("命中半径外不返回徽记"),
		FStrategySquadMarkerRules::FindHoveredMarker(Markers, FVector2D(200.0f, 200.0f), 17.0f), INDEX_NONE);
	return true;
}
```

- [ ] **Step 2: 写选择、命令、生命和模式失败测试**

继续在同一测试中加入：

```cpp
	TestTrue(TEXT("拖动已选小队时命令全部已选小队"),
		FStrategySquadMarkerRules::ShouldCommandSelectedSquads(true));
	TestFalse(TEXT("拖动未选小队时只命令源小队"),
		FStrategySquadMarkerRules::ShouldCommandSelectedSquads(false));
	TestEqual(TEXT("敌方目标转换为集火"),
		FStrategySquadMarkerRules::ResolveOrderType(true), EStrategyOrderType::AttackTarget);
	TestEqual(TEXT("地面目标转换为移动"),
		FStrategySquadMarkerRules::ResolveOrderType(false), EStrategyOrderType::Move);
	TestEqual(TEXT("一半生命与一次减员都保留初始生命上限"),
		FStrategySquadMarkerRules::CalculateHealthPercent(300.0f, 600.0f), 0.5f);
	TestFalse(TEXT("普通建筑放置时禁用徽记输入"), FStrategySquadMarkerRules::CanInteract(true, false));
	TestFalse(TEXT("城墙拖拽时禁用徽记输入"), FStrategySquadMarkerRules::CanInteract(false, true));
	TestTrue(TEXT("普通状态允许徽记输入"), FStrategySquadMarkerRules::CanInteract(false, false));
```

- [ ] **Step 3: 编译并确认 RED**

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\Build.bat' RTSEditor Win64 Development 'D:\ue project\RTS\RTS.uproject' -WaitMutex -NoHotReloadFromIDE
```

Expected: 编译因 `FStrategySquadMarkerRules` 尚不存在而失败。

- [ ] **Step 4: 实现最小纯规则接口**

在 `StrategySystems.h` 增加：

```cpp
struct FStrategySquadMarkerRules
{
	static int32 FindHoveredMarker(const TArray<FVector2D>& MarkerPositions,
		const FVector2D& CursorPosition, float HitRadius);
	static bool ShouldCommandSelectedSquads(bool bSourceSelected);
	static EStrategyOrderType ResolveOrderType(bool bVisibleEnemyTarget);
	static float CalculateHealthPercent(float CurrentHealth, float InitialTotalHealth);
	static bool CanInteract(bool bBuildingPlacementActive, bool bWallPlacementActive);
};
```

在 `StrategySystems.cpp` 使用平方距离寻找半径内最近索引；源小队已被选择时返回 true，否则返回 false。生命比例使用 `FMath::Clamp(CurrentHealth / InitialTotalHealth, 0.0f, 1.0f)`；两个建造模式均为 false 时才允许交互。

- [ ] **Step 5: 编译并运行规则测试确认 GREEN**

先执行 RTSEditor 编译，再运行：

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests RTS.Strategy.Systems.SquadMarkerRules;Automation TestExit" -log
```

Expected: `SquadMarkerRules` 为 `Result={Success}`，无 Fatal 或 Assert。

---

### Task 2: 单位生命查询与小队 HUD 数据

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyUnit.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`

**Interfaces:**
- Consumes: `FStrategySquadMarkerRules::CalculateHealthPercent(float, float)`
- Produces: `AStrategyUnit::GetCurrentHealth() const -> float`
- Produces: `AStrategySquad::GetHealthPercent() const -> float`
- Produces: `AStrategySquad::GetMarkerWorldLocation() const -> FVector`

- [ ] **Step 1: 暴露只读单位当前生命**

在 `AStrategyUnit` 的 public 区域增加：

```cpp
float GetCurrentHealth() const { return FMath::Max(0.0f, Health); }
```

不允许 HUD 或小队修改单位生命。

- [ ] **Step 2: 增加小队初始生命与查询接口**

在 `AStrategySquad` 增加：

```cpp
float GetHealthPercent() const;
FVector GetMarkerWorldLocation() const { return GetCenterLocation() + FVector(0.0f, 0.0f, 260.0f); }

float InitialTotalHealth = 0.0f;
```

- [ ] **Step 3: 初始化固定生命上限**

在 `AStrategySquad::Initialize()` 设置：

```cpp
InitialTotalHealth = Definition->MaxHealth * Definition->MemberCount;
```

该值之后不因成员受伤或死亡而修改。

- [ ] **Step 4: 聚合仍存活成员当前生命**

在 `StrategyWorldActors.cpp` 实现：

```cpp
float AStrategySquad::GetHealthPercent() const
{
	float CurrentHealth = 0.0f;
	for (const AStrategyUnit* Unit : Members)
	{
		if (IsValid(Unit))
		{
			CurrentHealth += Unit->GetCurrentHealth();
		}
	}
	return FStrategySquadMarkerRules::CalculateHealthPercent(CurrentHealth, InitialTotalHealth);
}
```

- [ ] **Step 5: 编译检查点**

运行 RTSEditor Win64 Development 编译。Expected: 编译成功，原有小队生成和死亡清理测试不回归。

---

### Task 3: 控制器投影命中与拖拽状态机

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`

**Interfaces:**
- Consumes: `FStrategySquadMarkerRules`、`AStrategySquad::GetMarkerWorldLocation()`、现有 `DoIssueOrder()`
- Produces: `FindSquadMarkerAtScreenPosition(const FVector2D&) const -> AStrategySquad*`
- Produces: `IsSquadMarkerDragging() const -> bool`
- Produces: `GetSquadDragSource() const -> AStrategySquad*`
- Produces: `GetSquadDragDestination() const -> FVector`
- Produces: `GetSquadDragTarget() const -> AActor*`
- Produces: `IsSquadSelected(AStrategySquad*) const -> bool`

- [ ] **Step 1: 增加控制器拖拽字段和 HUD 只读访问器**

在控制器头文件增加：

```cpp
UPROPERTY()
TObjectPtr<AStrategySquad> SquadMarkerSource;

UPROPERTY()
TObjectPtr<AActor> SquadDragTarget;

FVector2D SquadMarkerPressScreen = FVector2D::ZeroVector;
FVector SquadDragDestination = FVector::ZeroVector;
bool bSquadMarkerInputActive = false;
bool bSquadMarkerDragging = false;
bool bConsumeNextSelectClick = false;

bool IsSquadMarkerDragging() const { return bSquadMarkerDragging; }
AStrategySquad* GetSquadDragSource() const { return SquadMarkerSource; }
const FVector& GetSquadDragDestination() const { return SquadDragDestination; }
AActor* GetSquadDragTarget() const { return SquadDragTarget; }
bool IsSquadSelected(AStrategySquad* Squad) const { return ControlledSquads.Contains(Squad); }
AStrategySquad* FindSquadMarkerAtScreenPosition(const FVector2D& ScreenPosition) const;
```

- [ ] **Step 2: 实现投影命中**

`FindSquadMarkerAtScreenPosition()` 遍历 `AStrategyGameState::GetSquads()`，只收集 `Player`、存活、投影成功且投影点位于当前视口边界内的小队。将投影位置传给 `FindHoveredMarker(..., 17.0f)`，返回对应小队或 `nullptr`。不缓存跨帧屏幕位置。

- [ ] **Step 3: 在现有左键按下入口抢占徽记输入**

在 `SelectHoldStarted()` 中保持城墙模式为最高优先级；其后调用 `FStrategySquadMarkerRules::CanInteract(bBuildingPlacementActive, bWallPlacementActive)`。若命中徽记：

```cpp
SquadMarkerSource = FindSquadMarkerAtScreenPosition(GetMouseLocationForPlayer());
if (SquadMarkerSource)
{
	SquadMarkerPressScreen = GetMouseLocationForPlayer();
	bSquadMarkerInputActive = true;
	bConsumeNextSelectClick = true;
	return;
}
```

若普通建筑放置处于激活状态，则不检测徽记并直接返回，将输入留给现有建筑放置流程；`SelectHoldTriggered()` 和 `SelectHoldCompleted()` 在该模式下也直接返回，不更新框选。只有非建造状态且未命中徽记时才继续现有框选初始化。

- [ ] **Step 4: 实现 6 像素阈值与实时目标识别**

在 `SelectHoldTriggered()` 中，若 `bSquadMarkerInputActive`：

```cpp
const FVector2D Cursor = GetMouseLocationForPlayer();
bSquadMarkerDragging |= FVector2D::Distance(Cursor, SquadMarkerPressScreen) >= 6.0f;
if (bSquadMarkerDragging)
{
	UpdateSquadMarkerDragTarget();
}
return;
```

`UpdateSquadMarkerDragTarget()` 调用现有 `GetHitUnderCursor()`。仅当命中 Actor 实现 `IStrategyDamageable`、存活、非玩家阵营且 `GameState->IsVisibleToFaction(Player, ActorLocation)` 时设置 `SquadDragTarget`；否则清空目标并把 `Hit.Location` 写入 `SquadDragDestination`。

- [ ] **Step 5: 实现点击选择与松开命令**

在 `SelectHoldCompleted()` 的城墙分支之后、框选清理之前处理徽记状态：

```cpp
if (bSquadMarkerInputActive)
{
	if (!bSquadMarkerDragging)
	{
		const bool bAdditive = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
		if (!bAdditive)
		{
			DoDeselectAllUnitsCommand();
		}
		SelectSquad(SquadMarkerSource, bAdditive);
	}
	else
	{
		if (!FStrategySquadMarkerRules::ShouldCommandSelectedSquads(
			ControlledSquads.Contains(SquadMarkerSource)))
		{
			DoDeselectAllUnitsCommand();
			SelectSquad(SquadMarkerSource, false);
		}
		FStrategyOrder Order;
		Order.Type = FStrategySquadMarkerRules::ResolveOrderType(SquadDragTarget != nullptr);
		Order.TargetActor = SquadDragTarget;
		Order.Destination = SquadDragTarget ? SquadDragTarget->GetActorLocation() : SquadDragDestination;
		DoIssueOrder(Order);
	}
	ClearSquadMarkerInput();
	return;
}
```

`ClearSquadMarkerInput()` 只清空源、目标、目的地、`bSquadMarkerInputActive` 和 `bSquadMarkerDragging`，不得清除 `bConsumeNextSelectClick`。`SelectClick()` 与 `SelectClickAdditive()` 开头遇到 `bConsumeNextSelectClick` 时清除此标记并返回，防止 Enhanced Input 的普通或 Shift 点击完成事件再次执行地面选择。

- [ ] **Step 6: 模式切换时清理拖拽状态**

`HandleBuildMenuKey()` 和开始普通建筑/城墙放置的分支调用 `ClearSquadMarkerInput()`，确保建造模式不会保留半次小队拖拽。

- [ ] **Step 7: 编译检查点**

运行 RTSEditor Win64 Development 编译。Expected: 输入绑定不增加重复按键，现有框选、城墙拖拽和普通建筑放置代码继续编译。

---

### Task 4: HUD 徽记、生命条和拖拽反馈

**Files:**
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.h`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.cpp`

**Interfaces:**
- Consumes: `AStrategyGameState::GetSquads()`、小队 HUD 查询、控制器拖拽查询
- Produces: `DrawSquadMarker(...)`、`DrawDragArrow(...)`、`DrawCircle(...)`

- [ ] **Step 1: 声明三个紧凑绘制辅助函数**

在 `StrategyHUD.h` protected 区域增加：

```cpp
void DrawSquadMarker(const FVector2D& Center, float Diameter, EStrategyUnitType UnitType,
	float HealthPercent, bool bHighlighted);
void DrawDragArrow(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color);
void DrawCircle(const FVector2D& Center, float Radius, const FLinearColor& Color, float Thickness);
```

- [ ] **Step 2: 使用 Canvas 基础图元绘制徽记**

`DrawCircle()` 使用 16 段 `DrawLine` 逼近圆。`DrawSquadMarker()` 绘制半透明蓝色底、亮色圆边和下方 30×4 像素生命条：背景深灰，前景宽度为 `30.0f * HealthPercent`。步兵在圆内绘制盾形矩形，弓兵绘制弧线和弦，骑兵绘制双折线，不加载纹理。

- [ ] **Step 3: 绘制所有玩家小队徽记**

在 `DrawHUD()` 中遍历 `GameState->GetSquads()`。仅处理有效、存活、玩家阵营、`ProjectWorldLocationToScreen(GetMarkerWorldLocation())` 成功且投影点位于当前 Canvas 边界内的小队。使用控制器命中结果、选择状态和拖拽源决定直径：高亮为 42，否则 34。

- [ ] **Step 4: 绘制拖拽箭头与落点**

拖拽期间从源徽记中心绘制到当前鼠标位置。目标存在时使用红色，否则绿色。`DrawDragArrow()` 绘制主线和末端两个 12 像素箭头翼；将 `SquadDragDestination` 或目标 Actor 位置投影后，用同色 18 像素圆环标记落点。

- [ ] **Step 5: 更新底部操作提示**

普通状态 HUD 文案加入 `Drag squad badge to move/attack`。建造模式文案保持现有内容，不显示拖拽提示。

- [ ] **Step 6: 编译并运行完整编辑器测试**

运行 RTSEditor 编译和 `Automation RunTests RTS.Strategy`。Expected: 所有测试成功，日志无 Fatal 或 Assert。

---

### Task 5: PIE 验收、Windows 打包与冒烟

**Files:**
- Verify: `Source/RTS/Variant_Strategy/StrategySystems.*`
- Verify: `Source/RTS/Variant_Strategy/StrategyUnit.h`
- Verify: `Source/RTS/Variant_Strategy/StrategyWorldActors.*`
- Verify: `Source/RTS/Variant_Strategy/StrategyPlayerController.*`
- Verify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.*`
- Output: `Builds/Windows_SquadDragHUD/RTS.exe`

**Interfaces:**
- Consumes: Tasks 1–4 的全部接口
- Produces: 可运行 Windows Development 包

- [ ] **Step 1: 运行最终自动化验证**

执行完整 `RTS.Strategy` 测试并从日志统计 `Result={Success}`、失败数、Fatal 和 Assert。Expected: 失败数为 0，Fatal/Assert 为 0。

- [ ] **Step 2: PIE 人工验收单小队行为**

依次验证：单击徽记选择；Shift 单击追加/取消；拖动未选小队只移动该小队；地面目标为绿色；可见敌军和敌方建筑目标为红色并集火。

- [ ] **Step 3: PIE 人工验收多选和冲突**

框选至少两支小队后拖动任一已选徽记，确认全部移动且保持各自队形。进入 `B+5` 和 `B+6`，确认徽记仍绘制但不抢占普通建筑放置与城墙拖拽。退出建造后确认徽记恢复交互。

- [ ] **Step 4: PIE 人工验收显示状态**

缩放和平移镜头，确认徽记保持 34/42 像素屏幕尺寸并跟随小队。令一支小队受伤和减员，确认生命条以生成时初始总生命为固定上限；全灭后徽记消失。

- [ ] **Step 5: 构建 Windows Development 包**

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun -project='D:\ue project\RTS\RTS.uproject' -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory='D:\ue project\RTS\Builds\Windows_SquadDragHUD' -utf8output
```

Expected: `BUILD SUCCESSFUL`，`ExitCode=0`。

- [ ] **Step 6: 打包版启动冒烟**

启动 `Builds/Windows_SquadDragHUD/RTS.exe -windowed -ResX=1280 -ResY=720 -log`，保持运行至少 12 秒。确认日志出现 `Bringing World /Game/CityStateRTS/Maps/LVL_CityStateSkirmish`，且无 Fatal、Assert 或 `LogWindows: Error`，随后关闭该测试进程。
