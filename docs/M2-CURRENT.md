# M2 对战体验阶段记录

## 已完成

- M2-01：小队徽记单选、多选拖动、地面移动、敌军攻击和建造模式冲突已经实现并由用户确认正常。
- M2-02 自动部分：城门碰撞通道、敌军阻挡后攻击城门、墙门摧毁后的碰撞移除、迷雾显隐和索敌接线已核验。
- 修复普通右键可指定迷雾内敌军的问题。根因是 `SetActorHiddenInGame()` 只影响渲染，不关闭碰撞，而普通右键路径缺少可见性判断。
- 徽记拖动和普通右键现在共用“敌对、存活、当前可见”的攻击目标规则。

## 验证

- TDD RED：新增目标规则测试首次编译因 `FStrategyOrderTargetRules` 不存在而失败。
- 编辑器编译：`RTSEditor Win64 Development` 成功。
- 系统规则组：13 成功、0 失败、0 警告；包括 FogGrid、GateCollisionRules、BlockedGateTarget 和 OrderTargetRules。
- Windows 候选包：`Builds/Windows_M2_Candidate/RTS.exe`，`BUILD SUCCESSFUL`。
- 候选包持续运行 105 秒，进程存活；Fatal、访问冲突、Assert、生成碰撞失败和 Windows Error 共 0 条。

## 人工验收

用户确认以下项目均无问题：完工城门的阵营通行与敌军阻挡、敌军攻击城门、墙门摧毁后的缺口通行、迷雾目标限制，以及累计三局 AI 扩张、建造、训练、防守和进攻节奏。M2 已完成。
