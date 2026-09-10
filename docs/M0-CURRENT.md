# M0 当前版本核验

## 2026-09-05 本次改动

保留并核验工作区既有的小队拖拽改动：控制器处理徽记点击、拖拽和命令，HUD 绘制徽记、生命条、方向箭头与落点。用户已完成人工验收并确认交互正常。

修复长拖拽结束后第一次点击被吞的问题：IA_Strategy_SelectHold 使用 Hold，普通和追加点击使用 Tap；长拖拽不会产生 Tap，旧 bConsumeNextSelectClick 可残留。现在每次 SelectHoldStarted 开始时清除上一轮标记，再决定当前按下是否命中徽记。没有删除原有注释或 TODO。

回归测试直接驱动控制器的拖拽清理与下一次按下序列：修改前按预期失败，修改后 RTS.Strategy 全部 19 项成功、0 失败。报告位于 verification/M0-input-regression-red.json 和 M0-input-regression-green.json。该测试不替代真实 Enhanced Input/鼠标交互验收。

## B-004 退出异常结论

使用微软 WinDbg 中的 CDB 对旧候选包捕获到第一个访问异常，调用链为：

```text
ntdll!RtlEnterCriticalSection
RTS!TMallocBinnedCommon<FMallocBinned3,...>::FPerThreadFreeBlockLists::ClearTLS
RTS!FCsvProfilerProcessingThread::Run
RTS!FRunnableThreadWin::Run
```

访问地址为 0x24，发生于退出阶段的 CSV 处理线程 TLS 清理，随后崩溃报告线程异常导致外层退出码 777003。原始首异常堆栈摘录：verification/M0-csv-crash-stack.txt。此证据将问题定位到引擎 CSV/内存清理路径，不是小队玩法调用；具体线程销毁次序问题未修改引擎验证。

采用引擎源码现有的 csvNoProcessingThread 选项，将采集数据同步处理，保留 CSV 帧记录与错误检测，不关闭采集或崩溃处理。旧候选包在该选项下采集 300 帧、运行 60 秒后退出码 0。

脚本入口：scripts/Measure-Startup.ps1，默认测量 Builds/Windows_M0_Current。证据输出到 Saved/Verification/Startup-时间戳/。进程退出异常、未完成 CSV 或日志包含运行错误时，脚本明确失败。

```powershell
./scripts/Measure-Startup.ps1
```

同步 CSV 会改变游戏线程开销，今后的性能对比必须固定此模式。旧异步模式的帧时间不能与新模式直接比较。Hidden 窗口、无人操作、开局采样不代表前台整局或大规模交战表现。

默认异步 CSV 仍有可复现的引擎限制；本次提供的是项目采集流程的适配，不宣称修复了引擎。需使用异步采集时，再处理引擎升级或源代码修复。

## 验证与交付

- 编辑器编译：通过，Saved/Verification/M0-Current/Fixed-build.log。
- 规则测试：19/19 通过，Saved/Verification/M0-Current/Regression-green/。
- Windows 当前包：打包通过，BuildCookRun 退出码 0，输出 Builds/Windows_M0_Current/RTS.exe；交付文件摘要见 verification/M0-current-package.csv。
- 当前包脚本采集：通过；60 秒、300 帧、退出码 0、运行错误数 0，见 verification/M0-current-capture.json。原日志位于 Saved/Verification/Startup-20260905-125102/。
- 人工整局与交互：已验收；用户完成一整局并确认建造、训练、占领、战斗、迷雾、AI、胜负、R 重开及小队拖拽 HUD 正常。旧基线的“HUD 接入缺失”已被当前代码接入取代。

M0 已通过；M1 开局提示与整局阻断核验也已完成，后续状态以 ROADMAP.md 为准。

本轮源码快照见 verification/M0-current-source.csv，独立同盘备份位于 D:/ue project/RTS_Backups/M0-Current-20260905。引擎和工具的完整调试日志保留在 Saved/Verification/M0-Current，临时 WinDbg 仅解压在该目录，未安装到系统。
