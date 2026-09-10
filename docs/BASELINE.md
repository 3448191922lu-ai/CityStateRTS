# M0 基线与验收记录

日期：2026-09-05。当前基线已完成整局与人工交互验收，M0 已完成。任务状态以 ROADMAP.md 为准。

本文保留初始候选记录；最新拖拽代码修正、19 项测试及同步采集结果参见 M0-CURRENT.md。B-001/B-004 的当前处理状态以该文件及 ROADMAP.md 为准。

候选提交：127b32a。验证和安装包对应该提交中的游戏源码与资产。最终检查发现 StrategyPlayerController.cpp/.h 被其他操作加入拖拽实现，时间为 12:20；这些未提交变动未纳入本次验证。下面关于接入缺失的结论针对候选基线，不能直接用于判断后续工作区状态。本任务没有覆盖这些变动。

## 环境

- 引擎：D:/ue5/UE_5.8，Launcher 登记版本 5.8.2；项目 EngineAssociation 为 5.8。
- 编译目标：RTSEditor / RTS，Win64 Development。
- CPU：Intel Core i7-12700H；GPU：NVIDIA RTX 3050 Ti Laptop GPU；内存约 16 GB。
- 测量约定：1280×720 窗口；画质、实际单位规模和帧时间以采集记录为准。NullRHI 规则测试不能用作性能证据。

## 当前证据

| 检查 | 结果 | 证据 |
|---|---|---|
| 编辑器编译 | 已验收：退出码 0，Result: Succeeded | Build.bat RTSEditor Win64 Development，本次执行 |
| RTS.Strategy 自动化 | 已验收：18 成功，0 失败，0 未运行 | verification/M0-tests.json；原日志 Saved/Verification/M0/Automation.log |
| Windows 打包 | 已验收：BUILD SUCCESSFUL，退出码 0 | Saved/Verification/M0/Packaging.log；verification/M0-package.csv 校验 48 个交付文件 |
| 默认地图启动 | 已验收：默认地图加载，15 秒及 60 秒无采集复测正常退出 | Builds/Windows_M0/RTS.exe；Smoke-no-csv.log / Smoke-60-no-csv.log，退出码均为 0 |
| 60 秒采集退出 | 有缺陷：两轮退出码均为 777003 | Smoke.log / Smoke-repeat.log；同条件可复现，不能视为正常退出 |
| 整局与人工交互 | 已验收：用户完成一整局并确认核心系统与 R 重开正常 | 下方人工操作表及用户验收结论 |
| 启动性能采集 | 已记录，仅作参考 | verification/M0-startup-performance.json 和 M0-startup-frames.csv |
| Git/LFS | 配置已建立，候选提交保存于 baseline/m0 | 511 个 uasset/umap 使用 LFS；M0 验收前不打完成标签 |
| 源码、配置与资产快照 | 已验收：585 文件 SHA-256 一致 | D:/ue project/RTS_Backups/M0-20260905-0415；verification/M0-files.csv |

快照另外包含 RTS.uproject 和 CONTEXT.md。同盘快照用于版本恢复，不具备异盘灾备能力；当前未配置远端仓库或外置备份目的地。

## 功能验收表

以下状态是功能整体状态；规则单测通过不能替代场景集成验收。

| 功能 | 状态 | 已有证据 | 人工验收步骤 |
|---|---|---|---|
| 默认遭遇战地图 | 待验收 | 配置指向 LVL_CityStateSkirmish | 独立启动，看到双方主城、三个中立城镇及初始小队 |
| 收入与人口 | 待验收 | Economy / PopulationLimit / EarlyFactionInitialization 通过 | 观察收入，训练占人口，失去人口来源后不删除存量单位但限制继续训练 |
| 建造与训练 | 待验收 | BuildingFootprint / TrainingQueue 通过 | B 打开建造，1–6 选建筑并放置；选择生产建筑训练对应兵种，观察施工和队列 |
| 选择与命令 | 待验收 | 控制器中已有选择、命令逻辑 | 左键/框选、Shift 追加，右键移动/集火，F 后右键攻击移动，X 停止 |
| 三兵种战斗 | 待验收 | CounterDamage 通过 | 步弓骑分别训练并攻击，检查射程、伤害、死亡和人口释放 |
| 城镇占领与领地 | 待验收 | Capture 通过 | 单方驻留易主、双方暂停；占领后获得收入/领地，失去据点不销毁设施 |
| 战争迷雾 | 待验收 | FogGrid 通过 | 离开视野的敌军/设施隐藏，不能透过迷雾指定敌方目标 |
| 城墙与城门 | 待验收 | WallPlanner / WallPlacementRules / GateCollisionRules / BlockedGateTarget 通过 | 拖墙线，选择己方完成墙段 B+7 升门，验证友军通行、敌军受阻及破墙寻路 |
| 防御塔 | 待验收 | TowerTargetRules 通过 | 完成施工后攻击视野内敌军，不攻击友军或不可见目标 |
| AI | 待验收 | AIPriority 通过 | 观察占点、建造、训练、防守、进攻，记录时间线和是否僵持 |
| 镜头 | 待验收 | CameraMovement 通过 | WASD 平移、滚轮缩放、Z/C 旋转；检查选择反馈跟随 |
| 小队头顶拖拽 HUD | 有缺陷（接入缺失） | SquadMarkerRules 通过，但 HUD/控制器未检出拖拽接入 | 按现有 squad-drag-hud 设计补齐后验收；不可因规则单测通过标记完成 |
| 胜负与重开 | 待验收 | Victory 通过；HUD 已显示结果和 R/Q | 正常对战分别达到胜负，R 重新开局状态清空，Q 退出 |
| 暂停 | 待验收 | Esc 绑定 HandlePauseKey | 暂停后再次 Esc 恢复；确认暂停期间不推进战局 |

## 已知问题与下一阶段输入

- B-001：小队拖拽仅确认规则辅助实现，交互与绘制未接入。保留已有设计与 TODO；对应 M2-01，当前普通指挥是否可用需试玩。
- B-002：编辑器启动阶段出现 13 条 LogAutomationTest: Error: Condition failed，发生在 RTS.Strategy 执行前。该文本由引擎 LowLevelTestAdapter.h 的 CHECK 宏生成，具体触发调用尚未定位。项目独立报告仍为 18/18 成功，不能据此宣称全日志无错误。
- B-003：HandleRestartKey 固定打开 LVL_CityStateSkirmish；新增第二地图时需改为重开当前地图，列入 M3-01 验收。
- 编译环境警告：MSVC 14.51.36256 高于引擎首选 14.50；本次编辑器编译通过，暂不更换工具链。
- B-004（P0，待修复）：两次 60 秒 CSV 采集均在日志正常关闭后进程返回 777003。引擎 GenericPlatformCrashContext.h 将该值定义为 CrashReporterCrashed，WindowsPlatformCrashContext.cpp 在崩溃报告线程自身异常时返回该值。15 秒无采集及相同 60 秒无采集对照均退出码 0。当前关联到 CSV 采集条件，但缺少崩溃堆栈，不能断言具体故障函数。系统调试目录只有调试 DLL，未发现 cdb 可执行程序。下一步获取该复现的原始异常堆栈，再决定修复项目或处理引擎问题；不得以禁用崩溃处理隐藏异常。
- 初始全仓 Git 空白检查报告现有源码的行尾空格和末尾空行。本轮保持源码、注释和 TODO 原样；新增管理文档单独检查。

## 启动性能参考

首轮采集 300 帧，剔除最初 60 帧后剩余 240 帧：平均 9.843 ms，P95 11.228 ms，最大 30.793 ms；采样期间最多 8 名单位。D3D12、1280×720；查询到视距、阴影、全局光照、反射、后处理等级均为 3。

程序通过 Hidden 窗口方式启动且无人操作；样本只有开局数秒，首轮退出另有 B-004。以上数字不是整局、可见前台窗口或大规模交战的性能验收。完整原 CSV 和日志保存在快照目录，仓库保留逐帧摘录及统计。

## 可复现命令

在项目根目录使用 PowerShell；先创建日志目录，再依次执行。每步确认退出码与日志，失败不能继续宣称验收完成。

```powershell
New-Item -ItemType Directory -Force 'Saved/Verification/M0'
& 'D:/ue5/UE_5.8/Engine/Build/BatchFiles/Build.bat' RTSEditor Win64 Development '-Project=D:/ue project/RTS/RTS.uproject' -WaitMutex -NoHotReloadFromIDE -utf8output
& 'D:/ue5/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'D:/ue project/RTS/RTS.uproject' -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests RTS.Strategy' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/ue project/RTS/Saved/Verification/M0/Automation' '-abslog=D:/ue project/RTS/Saved/Verification/M0/Automation.log' -nosplash
& 'D:/ue5/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat' BuildCookRun '-project=D:/ue project/RTS/RTS.uproject' -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive '-archivedirectory=D:/ue project/RTS/Builds/Windows_M0' -utf8output
```

### B-004 复现条件

对打包后的 RTS/Binaries/Win64/RTS.exe 添加以下参数。日志目录使用 Saved/Verification/M0；启动时采用 Hidden 窗口，等待进程退出并记录真实 ExitCode。

```text
-windowed -ResX=1280 -ResY=720 -unattended -nosplash -seconds=60 -csvCaptureFrames=300 -csvCompression=0 -ExecCmds="obj list class=StrategyUnit,sg.ViewDistanceQuality,sg.ShadowQuality,sg.GlobalIlluminationQuality,sg.ReflectionQuality,sg.PostProcessQuality"
```

对照只移除 csvCaptureFrames 与 csvCompression 两个参数。两次采集退出码 777003，相同时长对照退出码 0。该实验不更改项目代码。

## 人工交付记录模板

每次试玩补充：日期、候选提交、地图、分辨率、画质、开局和峰值单位规模、帧时间、胜负/时长、复现步骤、截图或日志、验收人结论。初次基线至少检查上表全部功能，并完成胜利与失败路径。M2 再要求连续三局无阻断。
