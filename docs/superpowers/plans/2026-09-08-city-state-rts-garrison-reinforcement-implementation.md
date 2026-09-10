# 城邦争霸驻防与整补实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为主城和城镇增加最多 2/3 支小队驻防、免费整补、HUD 调出、受袭反击和公平 AI 使用闭环。

**Architecture:** `AStrategyControlPoint` 拥有驻军列表并更新整补与反击，`AStrategySquad` 保存驻防状态和计时，`AStrategyUnit` 负责成员级恢复与驻防显示状态。玩家控制器和 AI 只调用小队/据点共用接口；纯数值由 `FStrategyGarrisonRules` 统一计算。

**Tech Stack:** Unreal Engine 5.8、C++、UMG/Canvas HUD、UE Automation Tests。

**Spec:** `docs/superpowers/specs/2026-09-08-city-state-rts-garrison-reinforcement-design.md`

## Global Constraints

- 所有文件和文本使用 UTF-8；代码标识符使用英文，注释使用中文。
- 玩家与 AI 使用相同接口，不增加资源或视野作弊。
- 不改变现有占领、专精、补给、人口、战斗和胜负规则。
- 按 TDD 完成每个规则切片；阶段内只运行相关定向组，候选打包前只运行一次完整测试组。
- 不删除现有注释、文档注释或 TODO；不做无关重构。

---

### Task 1: 驻防纯规则与阶段登记

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyTerritoryTests.cpp`
- Modify: `docs/ROADMAP.md`
- Modify: `docs/M5-CURRENT.md`

**Interfaces:**
- Produces: `FStrategyGarrisonRules`，供据点、小队、控制器、HUD 和 AI 使用。

- [x] **Step 1:** 在测试中先调用尚不存在的 `GetCapacity`、`GetRecoveryDelay`、`GetRecoveryRate`、`GetReinforcementInterval`、`ShouldExitForDestination`、`ShouldAIRetreat` 和 `ShouldAILeave`。
- [x] **Step 2:** 编译确认测试因 `FStrategyGarrisonRules` 缺失而失败。
- [x] **Step 3:** 实现最小纯规则：普通/主城容量 2，激活要塞容量 3；普通等待 3 秒、3%/秒、8 秒补员；激活要塞立即、5%/秒、6 秒补员；目标距离大于 650 cm 出城；AI `<60%` 回城、`>=90%` 出城。
- [x] **Step 4:** 构建并只运行 `RTS.Strategy.Territory.GarrisonRules`，预期成功。
- [x] **Step 5:** 将 M5-05 登记为进行中，并把 M6-01 改为暂停等待 M5-05。

### Task 2: 成员生命恢复与小队驻防状态

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyUnit.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyUnit.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyTerritoryTests.cpp`

**Interfaces:**
- Produces: `AStrategyUnit::RestoreHealth`、`AStrategyUnit::SetGarrisoned`、`AStrategySquad::EnterGarrison`、`ExitGarrison`、`ApplyGarrisonRecovery`、`RestoreOneMember`、`IsGarrisoned` 和驻防计时查询。

- [x] **Step 1:** 添加成员恢复上限、小队恢复总量和补员不超过初始成员数的失败测试。
- [x] **Step 2:** 运行定向测试确认 RED。
- [x] **Step 3:** 为成员实现 `float RestoreHealth(float Amount)`；返回实际恢复量并限制到数据资产最大生命。
- [x] **Step 4:** 为成员实现 `SetGarrisoned(bool)`；驻防时停止移动、隐藏成员模型/骑手/选择环、关闭胶囊碰撞并停用战斗更新，出城时恢复原状态。
- [x] **Step 5:** 为小队保存 `GarrisonPoint`、恢复等待和补员计时；驻防时把成员移动到据点位置并进入隐藏状态，退出时在出口位置恢复。
- [x] **Step 6:** 按当前生命最低优先分配恢复量；通过比赛状态复用兵种定义生成一名满生命成员，保持原小队与人口占用不变。
- [x] **Step 7:** 运行新增小队/成员定向测试，预期成功。

### Task 3: 据点容量、整补与受袭反击

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyTerritoryTests.cpp`

**Interfaces:**
- Produces: `AStrategyControlPoint::TryGarrisonSquad`、`RemoveGarrisonedSquad`、`GetGarrisonedSquads`、`GetGarrisonCapacity`、`GetGarrisonMarkerWorldLocation` 和 `SortieGarrison`。

- [x] **Step 1:** 添加同阵营/存活/容量进入规则、要塞失效不驱逐超额驻军和入侵当帧暂停占领的失败测试。
- [x] **Step 2:** 运行定向测试确认 RED。
- [x] **Step 3:** 实现据点驻军列表；进入时拒绝敌方、全灭、重复和满员小队，普通/主城按 2、激活要塞按 3 判断。
- [x] **Step 4:** 在据点 Tick 中按当前专精规则更新每支驻军的恢复和补员计时。
- [x] **Step 5:** 计算现有 650 cm 范围内的非驻防双方小队；敌军首次进入所属据点范围时先暂停当帧占领，再将全部驻军从最近敌军方向的边缘位置调出并下达攻击移动。
- [x] **Step 6:** 为最多三支驻军提供确定性横向 HUD 槽位位置；绘制和命中共用该位置。
- [x] **Step 7:** 运行据点驻防定向测试，预期成功。

### Task 4: 玩家进出城命令与驻军 HUD

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.h`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUDRoot.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyHUDTests.cpp`

**Interfaces:**
- Consumes: Tasks 1-3 的驻防接口。
- Produces: 右键/徽记拖放入城、驻军徽记选择与范围外命令调出、底栏驻防状态。

- [x] **Step 1:** 添加命令解析失败测试：己方据点解析为驻防，驻军 650 cm 内命令保持驻防，范围外移动/攻击移动/集火先出城。
- [x] **Step 2:** 运行 UI 定向测试确认 RED。
- [x] **Step 3:** 在右键命中和徽记拖放流程中识别己方据点并逐队调用 `TryGarrisonSquad`；满员时使用现有通知与无效音效。
- [x] **Step 4:** `DoIssueOrder` 对驻军应用出城阈值；合法范围外命令调用 `ExitGarrison` 后继续原命令，停止和范围内命令不出城。
- [x] **Step 5:** HUD 使用据点提供的驻军槽位绘制和命中；隐藏模型期间保留己方徽记、生命条和拖动交互，敌方不显示。
- [x] **Step 6:** 底栏显示驻防据点、容量、等待/恢复/完成状态和整数秒补员倒计时；保持缓存更新，不按帧重建命令按钮。
- [x] **Step 7:** 构建并运行 `RTS.Strategy.UI`，预期 0 warning、0 failure。

### Task 5: 公平 AI 驻防整补

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyTerritoryTests.cpp`

**Interfaces:**
- Consumes: 共用据点驻防接口和 `FStrategyGarrisonRules` 的 60%/90% 阈值。
- Produces: AI 撤回最近有空位据点、恢复后出城并恢复现有战略决策。

- [x] **Step 1:** 添加 AI 阈值、最近可用据点和等距时要塞优先的失败测试。
- [x] **Step 2:** 运行定向测试确认 RED。
- [x] **Step 3:** 在防守威胁之后、发展和常规行动之前，令 `<60%` 的非驻防 AI 小队前往最近可用己方据点；不读取敌方驻军。
- [x] **Step 4:** 驻防 AI 小队达到 `>=90%` 时向现有集结/扩张目标方向出城；未达到阈值时不接受 `IssueAllSquads` 的常规命令。
- [x] **Step 5:** 运行 AI 驻防定向测试，预期成功。

### Task 6: 集成、候选包与人工验收

**Files:**
- Create: `docs/verification/M5-garrison-tests.json`
- Create: `docs/verification/M5-garrison-smoke.json`
- Create: `docs/verification/M5-garrison-package.csv`
- Create: `docs/M5-GARRISON-CURRENT.md`
- Modify: `docs/ROADMAP.md`
- Modify: `docs/M5-CURRENT.md`

**Interfaces:**
- Produces: `Builds/Windows_M5_Garrison` 候选及可复核证据。

- [x] **Step 1:** 构建 RTSEditor Development，运行一次完整 `RTS.Strategy` 自动化组并记录计数。
- [x] **Step 2:** BuildCookRun 到 `Builds/Windows_M5_Garrison`，预期 `BUILD SUCCESSFUL`。
- [x] **Step 3:** 启动内部程序 20 秒，检查崩溃、断言、项目资源加载和网络失败关键字。
- [x] **Step 4:** 生成排除运行日志的 SHA-256 文件清单并复核 0 不匹配。
- [ ] **Step 5:** 按最近五版本规则核对并清理超额旧候选。
- [ ] **Step 6:** 打开候选，等待玩家验证入城、HUD 调出、普通/要塞整补、容量、反击、主城驻防和 AI 回城；通过前 M5-05 保持待验收，M6 保持暂停。
