# 城墙、城门与箭塔实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在现有 UE5.8 RTS 原型中加入《帝国时代 IV》式拖拽城墙、墙段升级城门，并验证箭塔主动攻击行为。

**Architecture:** 使用纯 C++ 墙线规划器生成独立墙段，由 `AStrategyGameState` 统一执行合法性、消费、生成与城门替换。建筑 Actor 继续承载建造、生命、碰撞和战斗；玩家控制器只管理拖拽状态和预览，避免把规则写进输入层。

**Tech Stack:** Unreal Engine 5.8、C++、Enhanced Input、动态 Recast NavMesh、Automation Tests、UE MCP Data Asset 工具。

**Spec:** `docs/superpowers/specs/2026-09-04-wall-gate-tower-design.md`

## Global Constraints

- 所有文件和文本使用 UTF-8，代码标识符使用英文，注释使用中文。
- 城墙每段 75 金、8 秒、2000 生命；城门追加 100 金、10 秒、2200 生命。
- 箭塔 250 金、15 秒、1200 生命、1500 cm 射程、25 伤害/1.2 秒。
- 单次墙线最多 30 段；无效段跳过，金币不足时停止后续生成。
- 城门只能替换己方已完成墙段，建造中阻挡双方，完成后仅允许己方通过。
- 不实现登墙、墙头作战、端点堡垒、手动开门、自动闭合或 AI 墙线规划。
- 当前目录不是 Git 仓库；每个任务以编译与自动化测试成功作为可恢复检查点，不执行提交。

---

### Task 1: 纯规则墙线规划器

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Test: `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp`

**Interfaces:**
- Produces: `FStrategyWallSegmentPlan { FVector Location; FRotator Rotation; }`
- Produces: `FStrategyWallPlanner::BuildLine(const FVector& Start, const FVector& End, float SegmentLength, int32 MaxSegments) -> TArray<FStrategyWallSegmentPlan>`
- Produces: `FStrategyWallPlanner::GetAffordableCount(int32 ValidSegmentCount, float Gold, float SegmentCost) -> int32`

- [ ] **Step 1: 写失败测试**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyWallPlannerTest,
    "RTS.Strategy.Systems.WallPlanner",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyWallPlannerTest::RunTest(const FString& Parameters)
{
    const TArray<FStrategyWallSegmentPlan> Line = FStrategyWallPlanner::BuildLine(
        FVector::ZeroVector, FVector(2000, 0, 0), 400.0f, 30);
    TestEqual(TEXT("2000 cm 墙线生成五段"), Line.Num(), 5);
    TestEqual(TEXT("单次不超过三十段"), FStrategyWallPlanner::BuildLine(
        FVector::ZeroVector, FVector(20000, 0, 0), 400.0f, 30).Num(), 30);
    TestEqual(TEXT("200 金只能支付两段"),
        FStrategyWallPlanner::GetAffordableCount(5, 200.0f, 75.0f), 2);
    return true;
}
```

- [ ] **Step 2: 运行测试并确认 RED**

Run: `Build.bat RTSEditor Win64 Development "D:\ue project\RTS\RTS.uproject" -WaitMutex -NoHotReloadFromIDE`，随后运行 `Automation RunTests RTS.Strategy.Systems.WallPlanner`。

Expected: 测试因墙线规划接口尚未实现而失败。

- [ ] **Step 3: 实现最小规划规则**

```cpp
struct FStrategyWallSegmentPlan
{
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
};

struct FStrategyWallPlanner
{
    static TArray<FStrategyWallSegmentPlan> BuildLine(
        const FVector& Start, const FVector& End, float SegmentLength, int32 MaxSegments);
    static int32 GetAffordableCount(int32 ValidSegmentCount, float Gold, float SegmentCost);
};
```

实现时将起终点距离除以 400 cm，使用墙线中点作为各段位置，Yaw 对齐墙线方向，并将数量限制为 30。

- [ ] **Step 4: 运行 WallPlanner 测试并确认 GREEN**

Expected: `Result={Success}`，编辑器目标编译成功。

---

### Task 2: 建筑类型、矩形占地与数据资产

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyTypes.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.cpp`
- Create through UE MCP: `Content/CityStateRTS/Data/DA_Building_Wall.uasset`
- Create through UE MCP: `Content/CityStateRTS/Data/DA_Building_Gate.uasset`
- Test: `Source/RTS/Variant_Strategy/Tests/StrategyRulesTests.cpp`

**Interfaces:**
- Produces: `EStrategyBuildingType::Wall` and `EStrategyBuildingType::Gate`
- Produces: `UStrategyBuildingDataAsset::FootprintExtent` as `FVector2D`
- Consumes: existing `GetBuildingDefinition(EStrategyBuildingType)`

- [ ] **Step 1: 写失败测试**

增加旋转矩形占地测试：墙段中心与角点必须完整位于领地内，超出领地的角点必须令放置失败。期望调用：

```cpp
TestTrue(TEXT("墙段四角均在领地内时可建造"),
    FStrategyRules::IsFootprintInsideTerritory(
        FVector::ZeroVector, FVector2D(200.0f, 60.0f), 0.0f,
        FVector::ZeroVector, 1000.0f));
```

- [ ] **Step 2: 运行 Rules 测试并确认 RED**

Expected: 缺少矩形占地接口导致失败。

- [ ] **Step 3: 扩展类型和数据**

```cpp
enum class EStrategyBuildingType : uint8
{
    Barracks, ArcheryRange, Stable, House, Tower, Wall, Gate, Capital
};

UPROPERTY(EditAnywhere, BlueprintReadOnly)
FVector2D FootprintExtent = FVector2D(250.0f, 250.0f);
```

将现有建筑资产的 `FootprintRadius` 值迁移为等宽 `FootprintExtent`。墙段使用 `(200, 60)`，城门使用 `(200, 90)`。建筑路径表加载七个普通建筑定义。

- [ ] **Step 4: 通过 UE MCP 创建并保存资产**

`DA_Building_Wall`: `Wall`, `城墙`, 75, 8, 2000, `(200,60)`。

`DA_Building_Gate`: `Gate`, `城门`, 100, 10, 2200, `(200,90)`。

- [ ] **Step 5: 编译并运行全部 Rules 测试**

Expected: 新旧占地测试全部成功，资产加载日志无 `缺少建筑数据资产`。

---

### Task 3: 墙段批量建造与城门升级规则

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Test: `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp`

**Interfaces:**
- Produces: `TryPlaceBuilding(..., const FRotator& Rotation = FRotator::ZeroRotator)`
- Produces: `TryPlaceWallLine(EStrategyFaction, const FVector&, const FVector&) -> int32`
- Produces: `TryUpgradeWallToGate(EStrategyFaction, AStrategyBuilding*) -> AStrategyBuilding*`
- Produces: `AStrategyBuilding::IsConstructionComplete() const`

- [ ] **Step 1: 写失败测试**

使用纯批次筛选输入验证 `[valid, invalid, valid]` 保留首尾两段，并验证 150 金只生成两段。增加城门升级判定测试：敌方墙、未完成墙和非墙建筑均拒绝，己方完成墙允许。

- [ ] **Step 2: 运行 Systems 测试并确认 RED**

Expected: 批次筛选和城门升级判定接口尚不存在。

- [ ] **Step 3: 实现旋转单体放置和批量墙线提交**

`TryPlaceWallLine` 调用规划器，逐段执行矩形领地、导航地面与定向盒体重叠检查。无效段继续下一段；金币不足时结束循环；每个成功墙段独立扣除 75 金并生成 Actor。

- [ ] **Step 4: 实现原子城门升级**

先检查目标是己方完成墙段并确认可支付 100 金。生成同位置同旋转的城门后移除原墙段；城门从零建造进度开始且不退款。消费失败时不改变墙段。

- [ ] **Step 5: 运行相关测试与完整编译**

Expected: 墙线部分建造、无效段跳过和城门升级测试成功。

---

### Task 4: 墙体外观、碰撞与阵营城门

**Files:**
- Modify: `Config/DefaultEngine.ini`
- Modify: `Source/RTS/Variant_Strategy/StrategyUnit.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Test: `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp`

**Interfaces:**
- Produces: collision object channels `PlayerUnit` and `EnemyUnit`
- Produces: `FStrategyGateCollision::GetResponse(GateFaction, UnitFaction, bComplete) -> ECollisionResponse`
- Consumes: `AStrategyBuilding::CompleteConstruction()`

- [ ] **Step 1: 写失败测试**

```cpp
TestEqual(TEXT("未完成城门阻挡玩家"),
    FStrategyGateCollision::GetResponse(EStrategyFaction::Player, EStrategyFaction::Player, false), ECR_Block);
TestEqual(TEXT("完成城门放行己方"),
    FStrategyGateCollision::GetResponse(EStrategyFaction::Player, EStrategyFaction::Player, true), ECR_Ignore);
TestEqual(TEXT("完成城门阻挡敌方"),
    FStrategyGateCollision::GetResponse(EStrategyFaction::Player, EStrategyFaction::Enemy, true), ECR_Block);
```

- [ ] **Step 2: 运行测试并确认 RED**

Expected: 城门碰撞规则接口尚不存在。

- [ ] **Step 3: 配置单位碰撞通道并设置单位对象类型**

玩家和敌方单位初始化时分别使用 `PlayerUnit` 与 `EnemyUnit`。普通墙段阻挡两个通道并影响动态导航。

- [ ] **Step 4: 实现城门碰撞状态**

城门建造时阻挡两个通道；完成时忽略己方通道并阻挡敌方通道。城门不向动态导航注册为封闭障碍，使己方路径能够穿过门洞。

- [ ] **Step 5: 实现低多边形外观**

墙段使用横向 Cube；城门使用两个门柱和上梁组件，中部碰撞盒表达敌方阻挡。所有组件应用所属阵营材质。

- [ ] **Step 6: 编译并运行 GateCollision 测试**

Expected: 三种碰撞状态成功，原有单位与建筑测试不回归。

---

### Task 5: 敌军受阻后攻击城门

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyUnit.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyUnit.cpp`
- Test: `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp`

**Interfaces:**
- Produces: `FStrategyBlockingTargetRules::CanAttackBlocker(UnitFaction, BlockerFaction, bDamageable) -> bool`
- Produces: 单位胶囊碰撞命中回调，将敌方城门设为当前攻击目标

- [ ] **Step 1: 写失败测试**

验证敌方可伤害城门可作为阻挡目标，己方城门和不可伤害 Actor 不可作为阻挡目标。

- [ ] **Step 2: 运行测试并确认 RED**

Expected: 阻挡目标规则不存在。

- [ ] **Step 3: 接入碰撞命中与现有攻击流程**

单位碰到敌方 `AStrategyBuilding` 且其实现 `IStrategyDamageable` 时，把该建筑交给现有攻击目标逻辑；城门摧毁后清除目标并继续原命令。

- [ ] **Step 4: 编译并运行单位战斗测试**

Expected: 敌军可以破门，友军不会攻击己方城门。

---

### Task 6: 玩家拖拽、预览和建造菜单

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.cpp`

**Interfaces:**
- Consumes: `FStrategyWallPlanner::BuildLine`
- Consumes: `TryPlaceWallLine` and `TryUpgradeWallToGate`
- Produces: `BeginWallPlacement`, `UpdateWallPreview`, `CommitWallPlacement`, `ClearWallPreview`

- [ ] **Step 1: 绑定菜单入口**

添加数字键 `6` 和 `7` 处理函数。`B+5` 箭塔、`B+6` 城墙、`B+7` 城门；普通建筑保持原单击放置。

- [ ] **Step 2: 实现墙线拖拽状态**

城墙模式下左键按下保存地面起点，按住时更新终点，松开调用 `TryPlaceWallLine`。选择框逻辑在城墙模式下不执行。

- [ ] **Step 3: 实现预览**

根据共享规划与合法性检查生成临时 Cube 组件；有效段显示绿色，无效段显示红色。提交、取消或模式切换时销毁全部预览组件。

- [ ] **Step 4: 实现城门点击升级**

城门模式下光标必须命中己方完成墙段；调用升级接口成功后退出建造模式，失败时保持模式。

- [ ] **Step 5: 更新 HUD 文案**

建造栏显示 `[5] Tower  [6] Wall  [7] Gate`，城墙模式提示拖拽，城门模式提示点击己方完成墙段。

- [ ] **Step 6: 编译检查点**

Expected: RTSEditor Development 编译成功，输入绑定无重复键。

---

### Task 7: 箭塔可见目标规则、回归和交付

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Test: `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp`
- Output: `Builds/Windows/RTS.exe`

**Interfaces:**
- Produces: `FStrategyTowerTargetRules::CanTarget(bool bConstructionComplete, bool bAlive, bool bEnemy, bool bVisible, float Distance, float Range) -> bool`

- [ ] **Step 1: 写失败测试**

分别验证未建成、友军、不可见、超出 1500 cm 时返回 false，建成且可见的范围内敌军返回 true。

- [ ] **Step 2: 运行测试并确认 RED**

Expected: 箭塔目标规则接口尚不存在。

- [ ] **Step 3: 将规则接入箭塔索敌**

`UpdateTower` 使用目标规则和 `AStrategyGameState::IsVisibleToFaction`，保持 25 伤害与 1.2 秒攻击间隔。

- [ ] **Step 4: 运行完整自动化测试**

Run: `Automation RunTests RTS.Strategy`。

Expected: 所有测试 `Result={Success}`，失败数为 0，日志无 Fatal 或 Assert。

- [ ] **Step 5: PIE 功能验证**

验证直线、斜线、跨障碍墙线；己方通过完成城门；敌方受阻并攻击城门；墙段破坏形成缺口；箭塔只攻击范围内可见敌军。

- [ ] **Step 6: 构建 Windows Development 包**

Run: `RunUAT.bat BuildCookRun -project="D:\ue project\RTS\RTS.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="D:\ue project\RTS\Builds\Windows" -utf8output`

Expected: `BUILD SUCCESSFUL` 与 `ExitCode=0`。

- [ ] **Step 7: 打包版冒烟测试**

启动 `Builds/Windows/RTS.exe -windowed -ResX=1280 -ResY=720 -log`，保持运行至少 12 秒，确认默认地图加载完成且日志无 Fatal 或 Assert，再关闭测试进程。
