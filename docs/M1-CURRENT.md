# M1 完整一局阶段记录

## 结果

M1-01 与 M1-02 已完成。开局新增中央目标提示“占领城镇  发展军队  摧毁敌方主城”，前 7 秒完整显示，第 8 秒线性淡出，之后隐藏。底部快捷键提示、胜负界面、R 重开和 Q 退出保持原有行为。

M0 人工整局验收没有发现建造、训练、指挥、占领或战斗阻断项，因此 M1-02 无需增加修复代码。

## 验证

- TDD RED：新增测试首次编译因 `FStrategyIntroPromptRules` 不存在而失败。
- 编辑器编译：`RTSEditor Win64 Development` 成功。
- 单项测试：`RTS.Strategy.Systems.IntroPromptRules`，1 成功、0 失败、0 警告、0 错误。
- Windows Development 打包：`BUILD SUCCESSFUL`，AutomationTool 退出码 0，输出 `Builds/Windows_M1/RTS.exe`。
- 打包版启动：运行 12 秒时进程存活，运行日志中崩溃关键字 0 条。
- 视觉检查：`Builds/Windows_M1/RTS/Saved/Screenshots/Windows/HighresScreenshot00000.png`，中文字形正常，目标提示未遮挡资源栏或底部操作栏。

## 下一步

进入 M2-01，核验小队拖拽操作与视觉反馈。
