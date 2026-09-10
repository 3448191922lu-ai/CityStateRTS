# M3 内容阶段记录

## M3-01 河谷争渡候选版

- 新增第二张关卡 `LVL_RiverValleySkirmish`，原 `LVL_CityStateSkirmish` 保留。
- 河流纵向分割地图，北桥、中央浅滩和南桥形成三条通路。
- 三座中立城镇采用旋转对称布局：`(-2200, 3900)`、`(0, 0)`、`(2200, -3900)`。
- 河道、岩石和林地阻挡单位并参与动态导航；桥面和浅滩保留通行。
- 河道走廊禁止普通建筑、城墙和箭塔放置，避免封死桥梁或浅滩。
- 游戏模式、迷雾范围和初始出生点改由当前地图定义驱动。
- 胜负结束后的 `R` 键重新加载当前地图，不再固定返回第一张地图。
- M3-01 验收期间，编辑器和 Windows 包默认进入河谷地图。

## 自动验证

- TDD RED：地图定义测试首次编译因 `StrategyMapDefinition.h` 不存在而按预期失败。
- `RTSEditor Win64 Development` 编译成功。
- `RTS.Strategy.Systems`：15 项成功、0 项失败，包括新增的 MapDefinition 和 MapBuildRestriction。
- Windows Development 包：`Builds/Windows_M3_Candidate/RTS.exe`，`BUILD SUCCESSFUL`。
- 烘焙日志确认 `LVL_RiverValleySkirmish` 作为 World Partition 世界处理；项目目录同时保留两张 `.umap`。
- 独立包默认加载 `LVL_RiverValleySkirmish`，持续运行 25 秒；Fatal、未处理异常、断言、访问冲突及 SpawnActor 失败均为 0。
- 移动阻断修复包：`Builds/Windows_M3_NavFix4/RTS.exe`，`BUILD SUCCESSFUL`。
- 河谷关卡的 Recast 导航现启用 World Partition 导航，4 个导航数据块设为常驻；独立包运行时 4/4 数据块加载。
- 12 秒 AI 移动复现中，`InitPathfinding start point not on navmesh` 从修复前 88 次降为 0；Error 与 Fatal 均为 0。

## 已知边界

- 地形为基础几何体原型，正式河岸、桥梁和植被美术留到 M4。
- 本阶段没有选图菜单；第一张地图通过编辑器直接打开验证。
- 自动检查不能证明动态寻路和整局节奏，仍需人工试玩。
- 2026-09-06 用户已在 `Windows_M3_NavFix4` 中确认普通右键移动、小队徽记拖拽及拖至敌军自动攻击均无问题。

## 人工验收清单

- 河谷地图：小队可从北桥、中央浅滩和南桥通过，不能穿过其他河段。
- 河谷地图：河道、桥面和浅滩不能建造；两岸合法领地仍能正常建造。
- 河谷地图：AI 能占领、建造、训练、绕路、战斗并完成胜负流程。
- 河谷地图：胜负结束后按 `R` 仍重新进入河谷地图。
- 原地图：从编辑器打开 `LVL_CityStateSkirmish`，完整完成一局且布局与规则正常。

以上全部确认后，将 M3-01 从“待验收”改为“完成”，然后进入 M3-02 数值调整。

2026-09-06 更新：用户澄清两张地图验收没有问题，M3-01 已完成；导航移动修复也已完成人工验收。M3-02 的实际数值核对与后续对照场景见 M3-BALANCE.md。
