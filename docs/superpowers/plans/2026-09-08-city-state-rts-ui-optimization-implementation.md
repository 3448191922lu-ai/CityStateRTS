# 《城邦争霸》HUD 优化实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将现有屏幕 Canvas 文本与独立城镇面板整合为“城邦战术”风格的原生 UMG 主 HUD，同时完整保留世界投影、键鼠操作与玩法规则。

**Architecture:** 新增 `UStrategyHUDRoot` 负责顶部资源、通知、A1 平衡型整底栏和胜负覆盖层；`AStrategyHUD` 只保留框选、世界小队徽记、生命条、城镇图标、补给线和建造投影。UI 每帧读取控制器与比赛状态，所有按钮调用现有控制器和比赛状态路径，不复制业务规则。

**Tech Stack:** Unreal Engine 5.8、C++、UMG、Slate、Canvas HUD、UE Automation Framework、Windows BuildCookRun。

**Spec:** `docs/superpowers/specs/2026-09-08-city-state-rts-ui-optimization-design.md`

## Global Constraints

- 所有文件与文本使用 UTF-8；代码标识符使用英文，注释使用中文。
- 不安装 UI 插件，不引入外部字体、主题框架、小地图或新玩法。
- 不改变建造、训练、专精、迷雾、AI、伤害、胜负或资源规则。
- 世界小队徽记拖动、框选、左右键、WASD、滚轮、快捷键和暂停恢复必须保持现有行为。
- 1920×1080 为设计基准；人工验收覆盖 1280×720、1920×1080 与 1920×1200。
- 保留 `UStrategyTownPanel` 源文件和原有注释，但新版 HUD 验证期间不再实例化它。
- 当前工作区包含用户已有改动；不得覆盖无关文件，不创建 Git 提交。
- 按用户要求降低测试频率：各任务只运行自身定向验证，最终仅运行一次完整 `RTS.Strategy` 自动化组。

---

### Task 1: 建立纯 UI 状态模型与只读训练队列快照

**Files:**
- Create: `Source/RTS/Variant_Strategy/UI/StrategyHUDModel.h`
- Create: `Source/RTS/Variant_Strategy/UI/StrategyHUDModel.cpp`
- Create: `Source/RTS/Variant_Strategy/Tests/StrategyHUDTests.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `docs/ROADMAP.md`

**Interfaces:**
- Consumes: `FStrategyFactionState`、`FStrategyTrainingQueue`、现有选择与建造模式布尔状态。
- Produces: `EStrategyHUDContext`、`EStrategyHUDUnavailableReason`、`FStrategyHUDContextInputs`、`FStrategyHUDLayoutRules`、`FStrategyHUDActionRules`、`FStrategyTrainingQueue::GetItems()`、`FStrategyTrainingQueue::GetFrontProgress()`、`AStrategyBuilding::GetTrainingItems()` 与 `GetTrainingProgress()`。

- [x] **Step 1: 在路线图登记当前任务**

在 M5-03 后增加 `M5-04 | 领土深化/P1 | 进行中 | 统一城邦战术 HUD、上下文底栏、按钮反馈与分辨率适配 | M5-03 | 三种分辨率无重叠，输入与现有规则一致`。M5-01 至 M5-03 保持现有状态。

- [x] **Step 2: 添加失败的 UI 上下文与缩放测试**

在 `StrategyHUDTests.cpp` 添加：

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "StrategyHUDModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDContextRulesTest,
	"RTS.Strategy.UI.ContextRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDContextRulesTest::RunTest(const FString& Parameters)
{
	FStrategyHUDContextInputs Inputs;
	TestEqual(TEXT("无选择显示基础操作"), FStrategyHUDLayoutRules::ResolveContext(Inputs), EStrategyHUDContext::Idle);
	Inputs.SelectedSquadCount = 2;
	TestEqual(TEXT("小队上下文"), FStrategyHUDLayoutRules::ResolveContext(Inputs), EStrategyHUDContext::Squad);
	Inputs.bHasBuilding = true;
	TestEqual(TEXT("建筑优先于小队"), FStrategyHUDLayoutRules::ResolveContext(Inputs), EStrategyHUDContext::Building);
	Inputs.bHasTown = true;
	TestEqual(TEXT("城镇优先于建筑"), FStrategyHUDLayoutRules::ResolveContext(Inputs), EStrategyHUDContext::Town);
	Inputs.bBuildMode = true;
	TestEqual(TEXT("建造模式优先级最高"), FStrategyHUDLayoutRules::ResolveContext(Inputs), EStrategyHUDContext::Build);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDScaleRulesTest,
	"RTS.Strategy.UI.ScaleRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDScaleRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("720 高度使用最小缩放"), FStrategyHUDLayoutRules::ResolveScale(720.0f), 0.85f);
	TestEqual(TEXT("1080 高度使用基准缩放"), FStrategyHUDLayoutRules::ResolveScale(1080.0f), 1.0f);
	TestEqual(TEXT("1440 高度不无限放大"), FStrategyHUDLayoutRules::ResolveScale(1440.0f), 1.0f);
	return true;
}
#endif
```

- [x] **Step 3: 添加失败的训练快照与按钮原因测试**

继续在同一测试文件添加：

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDTrainingSnapshotTest,
	"RTS.Strategy.UI.TrainingSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDTrainingSnapshotTest::RunTest(const FString& Parameters)
{
	FStrategyTrainingQueue Queue;
	Queue.Enqueue(EStrategyUnitType::Infantry, 8.0f, 4);
	EStrategyUnitType CompletedType;
	int32 PopulationCost = 0;
	Queue.Update(2.0f, CompletedType, PopulationCost);
	TestEqual(TEXT("队列公开首项类型"), Queue.GetItems()[0].UnitType, EStrategyUnitType::Infantry);
	TestEqual(TEXT("首项训练进度"), Queue.GetFrontProgress(), 0.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDActionRulesTest,
	"RTS.Strategy.UI.ActionRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDActionRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("未建成建筑不可训练"), FStrategyHUDActionRules::GetTrainingUnavailableReason(
		false, true, 0, 500.0f, 0, 20, 100.0f, 4), EStrategyHUDUnavailableReason::UnderConstruction);
	TestEqual(TEXT("队列满优先提示"), FStrategyHUDActionRules::GetTrainingUnavailableReason(
		true, true, 5, 500.0f, 0, 20, 100.0f, 4), EStrategyHUDUnavailableReason::QueueFull);
	TestEqual(TEXT("金币不足"), FStrategyHUDActionRules::GetTrainingUnavailableReason(
		true, true, 0, 99.0f, 0, 20, 100.0f, 4), EStrategyHUDUnavailableReason::NotEnoughGold);
	TestEqual(TEXT("人口不足"), FStrategyHUDActionRules::GetTrainingUnavailableReason(
		true, true, 0, 500.0f, 17, 20, 100.0f, 4), EStrategyHUDUnavailableReason::PopulationFull);
	return true;
}
```

- [x] **Step 4: 编译一次，确认 RED**

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\Build.bat' RTSEditor Win64 Development 'D:\ue project\RTS\RTS.uproject' -WaitMutex -NoHotReloadFromIDE
```

Expected: 编译失败，错误仅来自 `StrategyHUDModel.h` 不存在或上述 UI 类型、方法尚未定义。

- [x] **Step 5: 实现最小 UI 模型接口**

在 `StrategyHUDModel.h` 定义：

```cpp
#pragma once

#include "CoreMinimal.h"

enum class EStrategyHUDContext : uint8
{
	Idle,
	Squad,
	Building,
	Town,
	Build
};

enum class EStrategyHUDUnavailableReason : uint8
{
	None,
	UnderConstruction,
	WrongBuilding,
	QueueFull,
	NotEnoughGold,
	PopulationFull
};

struct FStrategyHUDContextInputs
{
	bool bBuildMode = false;
	bool bHasTown = false;
	bool bHasBuilding = false;
	int32 SelectedSquadCount = 0;
};

struct FStrategyHUDLayoutRules
{
	static EStrategyHUDContext ResolveContext(const FStrategyHUDContextInputs& Inputs);
	static float ResolveScale(float ViewportHeight);
};

struct FStrategyHUDActionRules
{
	static EStrategyHUDUnavailableReason GetTrainingUnavailableReason(bool bConstructionComplete,
		bool bTrainable, int32 QueueLength, float Gold, int32 UsedAndReservedPopulation,
		int32 PopulationCap, float GoldCost, int32 PopulationCost);
	static FString GetUnavailableReasonText(EStrategyHUDUnavailableReason Reason);
};
```

在 `StrategyHUDModel.cpp` 按 `Build > Town > Building > Squad > Idle` 顺序解析上下文；缩放返回 `FMath::Clamp(ViewportHeight / 1080.0f, 0.85f, 1.0f)`。训练不可用原因按测试顺序返回，文字分别为“建筑尚未完工”“该建筑不能训练此单位”“训练队列已满”“金币不足”“人口已满”。

- [x] **Step 6: 暴露只读训练队列快照**

将 `FStrategyTrainingItem` 改为：

```cpp
struct FStrategyTrainingItem
{
	EStrategyUnitType UnitType = EStrategyUnitType::Infantry;
	float RemainingTime = 0.0f;
	float TotalTime = 0.0f;
	int32 PopulationCost = 0;
};
```

将入队初始化改为：

```cpp
Items.Add({UnitType, TrainingTime, TrainingTime, PopulationCost});
```

给 `FStrategyTrainingQueue` 添加：

```cpp
const TArray<FStrategyTrainingItem>& GetItems() const { return Items; }
float GetFrontProgress() const;
```

实现：

```cpp
float FStrategyTrainingQueue::GetFrontProgress() const
{
	return Items.IsEmpty() ? 0.0f
		: 1.0f - FMath::Clamp(Items[0].RemainingTime / Items[0].TotalTime, 0.0f, 1.0f);
}
```

给 `AStrategyBuilding` 添加只读转发：

```cpp
const TArray<FStrategyTrainingItem>& GetTrainingItems() const { return TrainingQueue.GetItems(); }
float GetTrainingProgress() const { return TrainingQueue.GetFrontProgress(); }
```

- [x] **Step 7: 运行一次 UI 定向测试并审查 Task 1**

Build expected: `Succeeded`。

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests RTS.Strategy.UI' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/ue project/RTS/Saved/Verification/M5/UIModel' '-abslog=D:/ue project/RTS/Saved/Verification/M5/UIModel.log' -nosplash
```

Expected: `RTS.Strategy.UI` 4/4 成功，0 warning、0 failure。审查队列 `Update`、人口释放和训练完成行为没有改变。

---

### Task 2: 创建 UMG 主 HUD 并替换旧屏幕 Canvas 文本

**Files:**
- Create: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.h`
- Create: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyHUDTests.cpp`

**Interfaces:**
- Consumes: Task 1 的上下文规则、缩放规则、训练快照，现有控制器选择 getter 与比赛状态 getter。
- Produces: `UStrategyHUDRoot::InitializeForController`、`Refresh`、`PushNotification`、A1 三列 HUD，以及 `AStrategyPlayerController::GetHUDRoot()`。

- [x] **Step 1: 添加 HUD 内容可见性契约测试**

在 `StrategyHUDModel.h` 增加 `FStrategyHUDVisibility`，在测试中先使用尚未实现的构造函数：

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDVisibilityTest,
	"RTS.Strategy.UI.Visibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDVisibilityTest::RunTest(const FString& Parameters)
{
	const FStrategyHUDVisibility Town = FStrategyHUDVisibility::ForContext(EStrategyHUDContext::Town);
	TestTrue(TEXT("城镇显示对象信息"), Town.bObjectPanel);
	TestTrue(TEXT("城镇显示详情"), Town.bDetailPanel);
	TestTrue(TEXT("城镇显示命令"), Town.bCommandPanel);
	TestFalse(TEXT("城镇不显示基础提示"), Town.bIdleHelp);
	const FStrategyHUDVisibility Idle = FStrategyHUDVisibility::ForContext(EStrategyHUDContext::Idle);
	TestTrue(TEXT("空选择显示基础提示"), Idle.bIdleHelp);
	return true;
}
```

- [x] **Step 2: 编译确认 RED，然后实现可见性值对象**

`FStrategyHUDVisibility` 包含 `bObjectPanel`、`bDetailPanel`、`bCommandPanel`、`bIdleHelp`。除 `Idle` 仅显示基础提示外，其他上下文均显示三列。重新编译，Expected: `Succeeded`；此时不运行完整自动化组。

- [x] **Step 3: 创建 `UStrategyHUDRoot` 的固定结构**

头文件公开接口：

```cpp
UCLASS()
class UStrategyHUDRoot : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForController(AStrategyPlayerController* InController);
	void Refresh();
	void PushNotification(const FString& Message, const FLinearColor& Color);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;

private:
	void RefreshResources();
	void RefreshContext();
	void RefreshMatchResult();
	void ApplyViewportScale();
	void ShowIdleContext();
	void ShowSquadContext();
	void ShowBuildingContext();
	void ShowTownContext();
	void ShowBuildContext();
	UButton* AddCommandButton(UHorizontalBox* Parent, const FString& Label);

	UPROPERTY(Transient) TObjectPtr<AStrategyPlayerController> Controller;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ResourceText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ObjectText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> CommandBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> NotificationBox;
	UPROPERTY(Transient) TObjectPtr<UOverlay> MatchOverlay;
};
```

`RebuildWidget()` 使用全屏 `UCanvasPanel`。顶部中央资源栏宽约 620、高 54；底栏距左右和底部 20 px、基准高 104 px。底栏为 `UHorizontalBox`，三个 `UHorizontalBoxSlot` 分别使用 `FillWidth(0.24f)`、`FillWidth(0.44f)`、`FillWidth(0.32f)`。背景色、金边、字体和内边距严格采用设计文档。

- [x] **Step 4: 实现顶部资源与只读上下文文本**

资源文字格式固定为：

```text
金币 {Gold}  +{Income}/秒    人口 {Used}+{Reserved}/{Cap}    城镇 {Owned}/{TotalNonCapital}
```

小队上下文显示已选数量、存活成员、初始成员和综合生命；综合生命继续使用 `AStrategySquad::GetHealthPercent()`，不得按幸存成员重新计算上限。建筑显示类型、生命、建造进度、队列长度和首项进度。城镇显示现有 `UStrategyTownPanel::Refresh` 中允许公开的同一信息，并继续调用 `FStrategyTownVisibilityRules` 防止迷雾泄漏。

本任务的命令区先显示按钮外观和快捷键，但点击事件留给 Task 3；键盘与鼠标原操作保持可用。

- [x] **Step 5: 在玩家控制器中接入根 HUD**

将控制器成员改为：

```cpp
class UStrategyHUDRoot;

UPROPERTY(Transient)
TObjectPtr<UStrategyHUDRoot> HUDRoot;

UStrategyHUDRoot* GetHUDRoot() const { return HUDRoot; }
```

在本地 `BeginPlay` 创建并以层级 20 加入玩家屏幕：

```cpp
HUDRoot = CreateWidget<UStrategyHUDRoot>(this, UStrategyHUDRoot::StaticClass());
HUDRoot->InitializeForController(this);
HUDRoot->AddToPlayerScreen(20);
```

停止创建 `UStrategyTownPanel`，并从 `SelectControlPoint` 移除对旧面板的 `ShowTown`/`HideTown` 调用；保留 `StrategyTownPanel.h/.cpp`。暂停菜单继续使用层级 100。

- [x] **Step 6: 将屏幕信息从 Canvas HUD 移走**

从 `AStrategyHUD::DrawHUD` 移除顶部资源矩形、底部文字栏和胜负矩形。完整保留框选、世界徽记、生命条、拖动箭头、城镇符号、工程进度与补给线绘制代码。

- [ ] **Step 7: 构建并做一次布局审查**

运行一次 RTSEditor Build，Expected: `Succeeded`。不运行完整测试。在 PIE 分别切换空选择、小队、建筑和城镇，确认只出现一个顶部栏和一个底栏，旧城镇面板没有实例化，世界徽记仍可拖动。

---

### Task 3: 接入命令按钮、不可用原因与建造位置反馈

**Files:**
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.h`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyTypes.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyHUDTests.cpp`

**Interfaces:**
- Consumes: Task 2 的命令容器、控制器已有 `Handle*` 与选择接口、比赛状态放置检查。
- Produces: `EStrategyBuildingPlacementIssue`、`FStrategyPlacementIssueRules`、`AStrategyGameState::GetBuildingPlacementIssue`、UI 命令包装方法和统一焦点恢复。

- [x] **Step 1: 添加失败的放置原因优先级测试**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDPlacementIssueTest,
	"RTS.Strategy.UI.PlacementIssue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDPlacementIssueTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("地图禁区优先"), FStrategyPlacementIssueRules::Resolve(false, false, false, true),
		EStrategyBuildingPlacementIssue::MapRestricted);
	TestEqual(TEXT("领地不足"), FStrategyPlacementIssueRules::Resolve(true, false, false, true),
		EStrategyBuildingPlacementIssue::OutsideTerritory);
	TestEqual(TEXT("不可导航"), FStrategyPlacementIssueRules::Resolve(true, true, false, true),
		EStrategyBuildingPlacementIssue::NotNavigable);
	TestEqual(TEXT("占地重叠"), FStrategyPlacementIssueRules::Resolve(true, true, true, true),
		EStrategyBuildingPlacementIssue::Overlap);
	TestEqual(TEXT("合法位置"), FStrategyPlacementIssueRules::Resolve(true, true, true, false),
		EStrategyBuildingPlacementIssue::None);
	return true;
}
```

- [x] **Step 2: 编译确认 RED，然后实现放置原因而不改变合法性**

在 `StrategyTypes.h` 添加：

```cpp
enum class EStrategyBuildingPlacementIssue : uint8
{
	None,
	MapRestricted,
	OutsideTerritory,
	NotNavigable,
	Overlap
};
```

在 `StrategyHUDModel.h/.cpp` 实现测试中的 `FStrategyPlacementIssueRules::Resolve`。在 `AStrategyGameState` 添加：

```cpp
EStrategyBuildingPlacementIssue GetBuildingPlacementIssue(EStrategyFaction Faction,
	EStrategyBuildingType BuildingType, const FVector& Location,
	const FRotator& Rotation = FRotator::ZeroRotator);
```

把现有 `CanPlaceBuilding` 的四项检查按原顺序移入该方法，并改为：

```cpp
return GetBuildingPlacementIssue(Faction, BuildingType, Location, Rotation)
	== EStrategyBuildingPlacementIssue::None;
```

地图、领地、导航与重叠判断的表达式必须原样复用，确保放置结果不变。

- [x] **Step 3: 为 UI 暴露同一路径命令包装方法**

在控制器 public 区添加：

```cpp
void BeginMoveCommandFromUI();
void BeginAttackMoveCommandFromUI();
void StopSelectedSquadsFromUI();
void ToggleBuildMenuFromUI();
void SelectBuildItemFromUI(int32 Index);
bool TrainSelectedBuildingFromUI(EStrategyUnitType UnitType);
void RestoreGameFocus();
uint8 GetPendingBuildingIndex() const { return PendingBuildingIndex; }
bool IsAttackMovePending() const { return bAttackMovePending; }
```

实现保持线性：移动按钮只清除 `bAttackMovePending` 并提示使用右键选择目标；攻击移动调用 `HandleAttackMoveKey`；停止调用 `HandleStopKey`；建造和编号按钮调用现有 `HandleBuildMenuKey`、`HandleNumberKey`；训练按兵种映射为 1–3 并复用 `SelectedBuilding->QueueUnit`。每个包装方法结束调用：

```cpp
void AStrategyPlayerController::RestoreGameFocus()
{
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
	bShowMouseCursor = true;
}
```

- [x] **Step 4: 绑定上下文按钮**

为 HUD 根控件创建按钮时保存按钮与文字指针。小队命令绑定移动、攻击移动、停止；建筑命令按 `Definition->TrainableUnits` 生成；建造菜单生成兵营、靶场、马厩、民居、箭塔、城墙和城门升级入口；城镇命令调用现有 `TrySpecializeSelectedTown` 与 `TryDowngradeSelectedTown`。

按钮启用状态由现有数据和 `FStrategyHUDActionRules` 决定。不可用按钮调用 `SetIsEnabled(false)` 并通过 `SetToolTipText` 显示一个原因；不得把成本或人口规则复制到点击回调中。

- [x] **Step 5: 显示建造位置原因**

仅在普通建筑放置模式中，使用当前鼠标命中位置调用 `GetBuildingPlacementIssue`。先检查玩家金币是否达到定义费用，再显示：

```text
✓ 领地、地形与占地均有效
金币不足
地图区域不可建造
超出己方领地
地面不可导航
与建筑或单位重叠
```

城墙拖动继续使用现有逐段绿/红预览，只在底栏显示“按住左键拖动，B 取消”，不改变 AoE4 式放置逻辑。

- [ ] **Step 6: 运行一次 UI 定向测试和输入检查**

Build expected: `Succeeded`。

Run `RTS.Strategy.UI` 到新的 `Saved/Verification/M5/UIInteraction` 报告目录。Expected: 6/6 成功，0 warning、0 failure。

在 PIE 只检查：按钮点击后 WASD 和滚轮立即恢复；右键移动、F+右键攻击移动、X 停止与按钮路径结果一致；城镇专精只扣一次金币；小队徽记拖动仍可下令。

---

### Task 4: 统一通知、胜负覆盖层与 Canvas 世界反馈

**Files:**
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDModel.h`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDModel.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.h`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyHUDTests.cpp`
- Create: `docs/verification/M5-ui-tests.json`

**Interfaces:**
- Consumes: HUD 根控件、现有 `PlayInvalidActionFeedback`、比赛胜负状态与 Canvas 世界绘制。
- Produces: `FStrategyHUDNotificationQueue`、`FStrategyFactionNotificationEvent`、最多三条的通知显示、中文胜负覆盖层和统一世界 UI 配色。

- [x] **Step 1: 添加失败的通知队列测试**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDNotificationQueueTest,
	"RTS.Strategy.UI.NotificationQueue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDNotificationQueueTest::RunTest(const FString& Parameters)
{
	FStrategyHUDNotificationQueue Queue;
	Queue.Push(TEXT("一"), FLinearColor::White);
	Queue.Push(TEXT("二"), FLinearColor::White);
	Queue.Push(TEXT("三"), FLinearColor::White);
	Queue.Push(TEXT("四"), FLinearColor::White);
	TestEqual(TEXT("最多三条"), Queue.GetItems().Num(), 3);
	TestEqual(TEXT("移除最旧通知"), Queue.GetItems()[0].Message, FString(TEXT("二")));
	Queue.Update(3.1f);
	TestTrue(TEXT("三秒后清空"), Queue.GetItems().IsEmpty());
	return true;
}
```

- [x] **Step 2: 编译确认 RED，然后实现通知队列**

定义：

```cpp
struct FStrategyHUDNotification
{
	FString Message;
	FLinearColor Color = FLinearColor::White;
	float RemainingSeconds = 3.0f;
};

struct FStrategyHUDNotificationQueue
{
	void Push(const FString& Message, const FLinearColor& Color);
	void Update(float DeltaSeconds);
	const TArray<FStrategyHUDNotification>& GetItems() const { return Items; }
private:
	TArray<FStrategyHUDNotification> Items;
};
```

`Push` 追加后从头移除到最多三条；`Update` 递减时间并移除到期项。

- [x] **Step 3: 将无效操作接入通知区**

把控制器中的 `PlayInvalidActionFeedback` 改为：

```cpp
void PlayInvalidActionFeedback(const FString& Message = TEXT("当前操作不可用"));
```

保留现有声音逻辑，并调用：

```cpp
if (HUDRoot)
{
	HUDRoot->PushNotification(Message, FLinearColor(0.93f, 0.34f, 0.28f, 1.0f));
}
```

放置失败、专精失败、降级失败、训练失败和城门升级失败分别传入 Task 1/3 已解析的单一原因。有效操作不弹出阻塞窗口。

- [x] **Step 4: 接入工程与训练完成通知**

在 `StrategyGameState.h` 添加轻量运行时通知事件：

```cpp
DECLARE_MULTICAST_DELEGATE_TwoParams(FStrategyFactionNotificationEvent,
	EStrategyFaction, const FString&);

// 添加到 public 区域。
FStrategyFactionNotificationEvent& OnFactionNotification() { return FactionNotification; }
void NotifyFaction(EStrategyFaction Faction, const FString& Message);

// 添加到现有 private 区域。
FStrategyFactionNotificationEvent FactionNotification;
```

`NotifyFaction` 只广播阵营和文本，不保存游戏状态。`UStrategyHUDRoot::InitializeForController` 使用 `AddUObject` 订阅，并在 `NativeDestruct` 中 `RemoveAll(this)`；HUD 只接收 `EStrategyFaction::Player`。

在既有完成分支中广播，不改动完成条件或资源结算：

- `AStrategyBuilding::CompleteConstruction`：`建筑已完工`。
- `FStrategyTrainingQueue::Update` 返回完成后的分支：`训练完成：步兵/弓兵/骑兵`。
- 城镇 `BuildCompleted`：`城镇专精建设完成`。
- 城镇 `DowngradeCompleted`：`城镇降级完成，返还 100 金币`。
- 城镇 `ReactivationCompleted`：`城镇已重新启用`。

- [x] **Step 5: 完成按钮样式、进度与资源警告**

按钮统一使用 `FButtonStyle`：正常深棕、悬停亮棕金边、按下深棕内缩、禁用灰色。生命条按 `> 0.5` 绿色、`> 0.25` 黄色、其余红色；训练和建设为金色，降级为橙色。资源不足或人口满时资源文字在 0.2 秒内短暂变红，然后恢复原色。

通知队列由 `NativeTick` 更新并重建最多三个文本项，剩余不足 1 秒时使用 `RemainingSeconds` 作为透明度。

- [x] **Step 6: 将胜负界面移入 UMG**

比赛结束时显示全屏低透明黑色覆盖层、中文“胜利”或“失败”，以及“R 重新开始 / Q 退出”。比赛运行时折叠覆盖层。继续使用现有 `HandleRestartKey` 和 `HandleQuitKey`，不增加鼠标按钮。

- [x] **Step 7: 统一 Canvas 世界反馈颜色**

只修改 `StrategyHUD.cpp` 中世界徽记、生命条、城镇图标、补给线和工程进度的颜色常量，使其与深蓝、金、绿、红、玩家蓝方案一致。不得改动投影位置、命中半径、拖动状态、迷雾条件或绘制顺序。

- [x] **Step 8: 运行一次最终完整自动化组**

先运行 RTSEditor Build，Expected: `Succeeded`。然后只运行一次：

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests RTS.Strategy' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/ue project/RTS/Saved/Verification/M5/UIFinal' '-abslog=D:/ue project/RTS/Saved/Verification/M5/UIFinal.log' -nosplash
```

Expected: 新增 `RTS.Strategy.UI` 7/7 成功，且全部 `RTS.Strategy` 测试成功，0 warning、0 failure。将结果摘要写入 `docs/verification/M5-ui-tests.json`。

---

### Task 5: UI 候选包、三分辨率人工验收与阶段文档

**Files:**
- Create: `docs/M5-UI-CURRENT.md`
- Modify: `docs/verification/M5-ui-tests.json`
- Create: `docs/verification/M5-ui-smoke.json`
- Create: `docs/verification/M5-ui-package.csv`
- Modify: `docs/ROADMAP.md`
- Modify: `docs/verification/M5-build-retention.json`

**Interfaces:**
- Consumes: Tasks 1–4 的集成 HUD 与最终自动化报告。
- Produces: `Builds/Windows_M5_UI`、哈希清单、启动证据、人工验收清单和 M5-04 路线图状态。

- [x] **Step 1: 将路线图任务更新为待验收**

将 Task 1 已登记的 M5-04 更新为 `待验收`，记录最终构建、测试、打包和人工 UI 检查结果。不把 M5-01 至 M5-03 提前标为完成。

- [x] **Step 2: 打包唯一 UI 候选**

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun '-project=D:/ue project/RTS/RTS.uproject' -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive '-archivedirectory=D:/ue project/RTS/Builds/Windows_M5_UI' -utf8output
```

Expected: `BUILD SUCCESSFUL`，且 `D:\ue project\RTS\Builds\Windows_M5_UI\RTS.exe` 存在。

- [x] **Step 3: 运行一次 20 秒打包烟雾检查**

使用内部程序 `Builds/Windows_M5_UI/RTS/Binaries/Win64/RTS.exe`、无空格绝对日志路径和 `Start-Process -PassThru`。保留精确 PID，20 秒后只停止该 PID。记录 `Fatal error`、`Unhandled Exception`、`Assertion failed`、`LowLevelFatalError`、项目资源加载失败、网络失败和普通 Error 行；Expected: 均为 0。

- [x] **Step 4: 生成不可变发布文件清单**

递归枚举 `Builds/Windows_M5_UI`，排除启动后生成的 `RTS/Saved/*` 与 `RTS/Binaries/Win64/RTS.log`，按相对路径排序，将路径、字节数和 SHA-256 写入 UTF-8 `docs/verification/M5-ui-package.csv`。重新计算并确认清单中每一行匹配实际文件。

- [ ] **Step 5: 只保留最新五个 Windows 候选**

先只读列出 `Builds` 下名称以 `Windows_` 开头的目录，按 `LastWriteTime` 降序保留五个。对每个拟删除目录重新解析绝对路径，确认父目录严格等于 `D:\ue project\RTS\Builds`、叶名称以 `Windows_` 开头且不等于 `Windows_M5_UI`，然后才逐个使用 `Remove-Item -LiteralPath` 删除。更新 `M5-build-retention.json`，明确删除不可恢复。

- [ ] **Step 6: 完成三分辨率人工验收**

只检查 UI，不重复整局：

1. 1280×720、1920×1080、1920×1200 均无重叠、裁切或异常竖排。
2. 空选择、小队、建筑、城镇和建造模式均保持 24/44/32 三列布局。
3. 城镇贸易、征募、要塞和降级按钮与原规则一致，只扣费一次。
4. 建筑训练队列、生命和进度正确，禁用原因明确。
5. WASD、滚轮、框选、左右键、F、X、B、Esc 暂停恢复和小队徽记拖动正常。
6. 敌方城镇和迷雾信息没有新增泄漏。

- [x] **Step 7: 写入最终证据并等待用户验收**

在 `M5-UI-CURRENT.md` 记录设计选择、代码接口、构建、自动化、烟雾、包清单、分辨率结果和已知限制。将 M5-04 设为 `待验收`；只有用户确认 UI 候选后才改为 `完成`。M5 尚未整体验收时不得启动 M6-01。

- [x] **Step 8: 最终一致性检查**

Run:

```powershell
git diff --check
rg -n "UStrategyHUDRoot|EStrategyHUDContext|GetTrainingProgress|GetBuildingPlacementIssue|Windows_M5_UI|M5-04" Source docs
```

Expected: 无空白错误；允许单独报告现有 CRLF/LF 转换警告。确认设计文档、计划、源码接口、路线图、候选路径和验证证据一致。
