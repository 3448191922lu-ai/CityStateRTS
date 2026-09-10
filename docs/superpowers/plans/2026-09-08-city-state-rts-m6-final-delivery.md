# 城邦争霸 M6 最终交付实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将已完成人工验收的 M5 候选冻结为可独立启动、可校验、可交付的 Windows 单人试玩版。

**Architecture:** 不新增玩法代码。沿用现有 `RTS.Strategy` 自动化、Windows Development BuildCookRun、短时启动检查和 SHA-256 清单；最终人工验收仍由玩家在两张地图中完成。

**Tech Stack:** Unreal Engine 5.8、C++、UMG、PowerShell、AutomationTool。

**Spec:** `docs/ROADMAP.md`

## Global Constraints

- 所有文件和文本使用 UTF-8；代码标识符使用英文，注释使用中文。
- 不修改已经验收的玩法规则，不增加插件或公共框架。
- 减少测试频率：仅在最终打包前运行一次完整自动化组，打包后只做一次短时启动检查。
- `Builds` 下只保留最近五个 `Windows_*` 候选，删除前必须核对绝对路径和直接父目录。
- 保留现有注释、文档注释和 TODO。

---

### Task 1: 冻结 M4 与 M5 验收状态

**Files:**
- Modify: `docs/ROADMAP.md`
- Modify: `docs/M4-SETTINGS-CURRENT.md`
- Modify: `docs/M5-CURRENT.md`
- Modify: `docs/M5-UI-CURRENT.md`

**Interfaces:**
- Consumes: 用户对设置流程、城镇深化、AI、三分辨率 HUD 与字体稳定性的人工确认。
- Produces: M4-02、M5-01 至 M5-04 的完成状态，以及 M6-01 进行中状态。

- [x] **Step 1:** 将已获得的人工验收结果写入各当前状态文档。
- [x] **Step 2:** 将路线图对应任务改为完成，并把 M6-01 改为进行中。
- [x] **Step 3:** 检查文档中不再保留与当前事实冲突的“待验收”描述。

### Task 2: 最终自动化与性能基线

**Files:**
- Create: `docs/verification/M6-tests.json`
- Create: `docs/verification/M6-performance.json`

**Interfaces:**
- Consumes: `RTS.Strategy` 自动化组和现有同步 CSV 性能采集路径。
- Produces: 最终测试计数、1280×720 固定条件下的实际帧时间与运行错误摘要。

- [x] **Step 1:** 构建 RTSEditor Development，预期成功。
- [x] **Step 2:** 运行一次完整 `RTS.Strategy` 自动化并导出报告，预期 0 warning、0 failure。
- [x] **Step 3:** 在 1280×720 窗口使用同步 CSV 路径采集 60 秒，记录实际样本，不凭启动成功宣称性能。
- [x] **Step 4:** 将测试和性能摘要写入 UTF-8 JSON。

### Task 3: 构建最终 Windows 候选

**Files:**
- Create: `Builds/Windows_M6_Final/`
- Create: `docs/verification/M6-smoke.json`
- Create: `docs/verification/M6-package.csv`

**Interfaces:**
- Consumes: Task 2 已验证的源码、配置和资产。
- Produces: Windows Development 最终候选、启动证据和逐文件 SHA-256 清单。

- [x] **Step 1:** 使用 BuildCookRun 构建并归档到 `Builds/Windows_M6_Final`，预期 `BUILD SUCCESSFUL`。
- [x] **Step 2:** 启动内部程序 20 秒，检查崩溃、断言、项目资源加载和网络失败关键字。
- [x] **Step 3:** 排除运行日志后生成路径、字节数和 SHA-256 清单，并逐行复核为 0 不匹配。

### Task 4: 版本清理与交付说明

**Files:**
- Modify: `docs/verification/M5-build-retention.json`
- Create: `docs/M6-CURRENT.md`
- Modify: `docs/ROADMAP.md`

**Interfaces:**
- Consumes: 新最终候选和现有 `Builds/Windows_*` 目录。
- Produces: 最近五个候选目录、最终运行说明、已知问题和两地图人工验收清单。

- [x] **Step 1:** 按更新时间列出所有 Windows 候选，解析并核对拟删除目录严格位于 `Builds` 下。
- [x] **Step 2:** 删除超过最近五个的旧候选，并记录实际保留与删除项。
- [x] **Step 3:** 写明启动路径、键位、玩法目标、设置入口、已知限制和校验方法。
- [ ] **Step 4:** 打开最终包，等待玩家分别完成两张地图的一局验收；通过前 M6-01 保持待验收。
