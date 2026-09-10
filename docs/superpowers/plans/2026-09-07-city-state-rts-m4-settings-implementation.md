# M4-02 Audio and Display Settings Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a persistent in-match pause menu with master volume, window mode, resolution, and overall quality settings to the Windows RTS prototype.

**Architecture:** `UStrategyGameUserSettings` extends Unreal's existing settings object and adds only the project master-volume value. A native `UStrategyPauseMenu` owns draft editing and UMG presentation, while `AStrategyPlayerController` owns menu lifetime, pause state, and input-mode switching.

**Tech Stack:** Unreal Engine 5.8 C++, UGameUserSettings, UMG/Slate, Automation Tests, Windows BuildCookRun.

**Spec:** `docs/superpowers/specs/2026-09-07-city-state-rts-m4-settings-design.md`

## Global Constraints

- All files and text remain UTF-8; identifiers are English and new comments are Chinese.
- Scope is limited to master volume, window mode, supported resolutions, and Low/Medium/High/Epic overall quality.
- Do not add plugins, third-party dependencies, background music, category volumes, per-setting graphics controls, key rebinding, language options, or a standalone main menu.
- Preserve all existing gameplay behavior and all existing comments/TODO markers.
- Keep the mouse visible during normal RTS play; use `UIOnly` while the pause menu is open so commands cannot pass through.
- Use targeted tests and one packaged-candidate smoke check; do not rerun the full gameplay suite unless a targeted failure indicates wider regression.
- The checkout contains substantial user-owned changes. Do not reset, delete, or commit unrelated files; do not create commits from this shared dirty checkout.

---

### Task 1: Persistent strategy user settings

**Files:**
- Create: `Source/RTS/Variant_Strategy/StrategyGameUserSettings.h`
- Create: `Source/RTS/Variant_Strategy/StrategyGameUserSettings.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp`

**Interfaces:**
- Consumes: Unreal `UGameUserSettings`, `EWindowMode::Type`, `FIntPoint`, and `FApp::SetVolumeMultiplier`.
- Produces: `FStrategySettingsDraft`, `FStrategySettingsRules`, and `UStrategyGameUserSettings` for the pause menu.

- [x] **Step 1: Add the failing settings-rule test**

Add `#include "StrategyGameUserSettings.h"` and register `RTS.Strategy.Systems.SettingsRules`. The test must contain these assertions before the production header exists:

```cpp
TestEqual(TEXT("负音量限制为零"), FStrategySettingsRules::SnapMasterVolume(-0.3f), 0.0f);
TestEqual(TEXT("音量按百分之五取整"), FStrategySettingsRules::SnapMasterVolume(0.53f), 0.55f);
TestEqual(TEXT("过高音量限制为一"), FStrategySettingsRules::SnapMasterVolume(1.4f), 1.0f);
TestEqual(TEXT("低画质映射"), FStrategySettingsRules::QualityIndexToLevel(0), 0);
TestEqual(TEXT("史诗画质映射"), FStrategySettingsRules::QualityIndexToLevel(3), 3);
TestEqual(TEXT("窗口模式映射"), FStrategySettingsRules::WindowModeIndexToValue(0), EWindowMode::Windowed);
TestEqual(TEXT("无边框模式映射"), FStrategySettingsRules::WindowModeIndexToValue(1), EWindowMode::WindowedFullscreen);
TestEqual(TEXT("独占全屏映射"), FStrategySettingsRules::WindowModeIndexToValue(2), EWindowMode::Fullscreen);
TestEqual(TEXT("分辨率显示"), FStrategySettingsRules::FormatResolution(FIntPoint(1920, 1080)), FString(TEXT("1920 x 1080")));
```

- [x] **Step 2: Build once to verify RED**

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\Build.bat' RTSEditor Win64 Development '-Project=D:/ue project/RTS/RTS.uproject' -WaitMutex -NoHotReloadFromIDE -utf8output
```

Expected: compile failure because `StrategyGameUserSettings.h` and its interfaces do not exist.

- [x] **Step 3: Implement the minimal settings types**

Create the header with these exact public interfaces:

```cpp
struct FStrategySettingsDraft
{
	float MasterVolume = 1.0f;
	EWindowMode::Type WindowMode = EWindowMode::WindowedFullscreen;
	FIntPoint Resolution = FIntPoint::ZeroValue;
	int32 OverallQuality = 3;
};

struct FStrategySettingsRules
{
	static float SnapMasterVolume(float Value);
	static int32 QualityIndexToLevel(int32 Index);
	static EWindowMode::Type WindowModeIndexToValue(int32 Index);
	static int32 WindowModeValueToIndex(EWindowMode::Type Value);
	static FString FormatResolution(const FIntPoint& Resolution);
	static bool ParseResolution(const FString& Text, FIntPoint& OutResolution);
};

UCLASS(Config=GameUserSettings)
class UStrategyGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static UStrategyGameUserSettings* Get();
	virtual void SetToDefaults() override;
	virtual void ApplyNonResolutionSettings() override;
	float GetMasterVolume() const { return MasterVolume; }
	void SetMasterVolume(float Value);
	FStrategySettingsDraft MakeDraft() const;
	void ApplyDraft(const FStrategySettingsDraft& Draft);

private:
	UPROPERTY(Config)
	float MasterVolume = 1.0f;
};
```

Implement volume snapping with `FMath::RoundToFloat(FMath::Clamp(Value, 0.0f, 1.0f) * 20.0f) / 20.0f`. `SetToDefaults()` calls `Super`, then selects desktop resolution, `WindowedFullscreen`, quality `3`, and volume `1.0f`. `ApplyNonResolutionSettings()` calls `Super` then `FApp::SetVolumeMultiplier(MasterVolume)`.

`MakeDraft()` copies current settings without mutating them. `ApplyDraft()` stores the snapped volume, resolution, window mode and quality, calls `ApplySettings(false)`, confirms the video mode, and saves settings. Before applying, retain the previous resolution and window mode. After applying, compare `GSystemResolution` and the game viewport's window mode with the request; if they do not match, restore and reapply the previous display pair before saving.

- [x] **Step 4: Build and run the targeted GREEN test**

Run the editor build command from Step 2, then:

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests RTS.Strategy.Systems.SettingsRules' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/ue project/RTS/Saved/Verification/M4/SettingsRules' '-abslog=D:/ue project/RTS/Saved/Verification/M4/SettingsRules.log' -nosplash
```

Expected: `1` succeeded, `0` failed.

---

### Task 2: Native pause-menu state and controls

**Files:**
- Create: `Source/RTS/Variant_Strategy/UI/StrategyPauseMenu.h`
- Create: `Source/RTS/Variant_Strategy/UI/StrategyPauseMenu.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp`

**Interfaces:**
- Consumes: `FStrategySettingsDraft`, `FStrategySettingsRules`, `UStrategyGameUserSettings`, and `AStrategyPlayerController` callbacks.
- Produces: `UStrategyPauseMenu::OpenPausePage()`, `UStrategyPauseMenu::OpenSettingsPage()`, `UStrategyPauseMenu::HandleEscape()`, and `EStrategyPauseMenuPage`.

- [x] **Step 1: Add the failing menu-flow test**

Add `#include "StrategyPauseMenu.h"` and register `RTS.Strategy.Systems.SettingsMenuFlow` with:

```cpp
TestEqual(TEXT("暂停页 Esc 关闭菜单"),
	FStrategyPauseMenuRules::ResolveEscape(EStrategyPauseMenuPage::Pause),
	EStrategyPauseMenuEscapeAction::CloseMenu);
TestEqual(TEXT("设置页 Esc 放弃并返回"),
	FStrategyPauseMenuRules::ResolveEscape(EStrategyPauseMenuPage::Settings),
	EStrategyPauseMenuEscapeAction::DiscardAndReturn);

TArray<FIntPoint> Resolutions = { FIntPoint(1920, 1080), FIntPoint(1280, 720), FIntPoint(1920, 1080) };
FStrategyPauseMenuRules::NormalizeResolutions(Resolutions, FIntPoint(1600, 900));
TestEqual(TEXT("分辨率去重并保留当前项"), Resolutions.Num(), 3);
TestEqual(TEXT("分辨率按面积排序"), Resolutions[0], FIntPoint(1280, 720));
TestTrue(TEXT("当前分辨率存在"), Resolutions.Contains(FIntPoint(1600, 900)));
```

- [x] **Step 2: Build once to verify RED**

Run the Task 1 editor build command.

Expected: compile failure because `StrategyPauseMenu.h` does not exist.

- [x] **Step 3: Implement menu state and native UMG tree**

Create these exact types:

```cpp
UENUM()
enum class EStrategyPauseMenuPage : uint8 { Pause, Settings };

enum class EStrategyPauseMenuEscapeAction : uint8 { CloseMenu, DiscardAndReturn };

struct FStrategyPauseMenuRules
{
	static EStrategyPauseMenuEscapeAction ResolveEscape(EStrategyPauseMenuPage Page);
	static void NormalizeResolutions(TArray<FIntPoint>& Resolutions, const FIntPoint& CurrentResolution);
};

UCLASS()
class UStrategyPauseMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForController(AStrategyPlayerController* InController);
	void OpenPausePage();
	void OpenSettingsPage();
	void HandleEscape();
	EStrategyPauseMenuPage GetCurrentPage() const { return CurrentPage; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
};
```

Build a centered `UCanvasPanel`/`UBorder`/`UVerticalBox` hierarchy in `RebuildWidget()`. The pause page contains the title `城邦争霸`, plus `继续游戏`, `设置`, and `退出游戏` buttons. The settings page contains:

```text
设置
总音量    [slider] 100%
窗口模式  [窗口 / 无边框全屏 / 独占全屏]
分辨率    [supported resolution combo]
画质预设  [低 / 中 / 高 / 史诗]
[应用并返回] [放弃并返回]
```

Use dynamic delegates bound to `UFUNCTION()` handlers. `OpenSettingsPage()` obtains `UStrategyGameUserSettings::Get()->MakeDraft()`, fills the controls, and enumerates resolutions with `UKismetSystemLibrary::GetSupportedFullscreenResolutions`. `NormalizeResolutions()` sorts by `X * Y`, removes duplicates, and inserts the current resolution if absent. Slider updates only the draft and percentage label. Combo selections update only the draft.

`ApplyAndReturn` calls `UStrategyGameUserSettings::Get()->ApplyDraft(Draft)` and returns to the pause page. `DiscardAndReturn` replaces the draft with `MakeDraft()` and returns without applying. `NativeOnKeyDown` handles Escape through `FStrategyPauseMenuRules` and calls the controller to close only from the pause page.

- [x] **Step 4: Build and run the targeted menu-flow test**

Run the editor build, then run only:

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests RTS.Strategy.Systems.SettingsMenuFlow' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/ue project/RTS/Saved/Verification/M4/SettingsMenuFlow' '-abslog=D:/ue project/RTS/Saved/Verification/M4/SettingsMenuFlow.log' -nosplash
```

Expected: `1` succeeded, `0` failed.

---

### Task 3: Player-controller pause and input integration

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyPauseMenu.cpp`

**Interfaces:**
- Consumes: `UStrategyPauseMenu` and existing build/selection cleanup methods.
- Produces: `AStrategyPlayerController::OpenPauseMenu()`, `ClosePauseMenu()`, `QuitFromPauseMenu()`, and `IsPauseMenuOpen()`.

- [x] **Step 1: Add the controller-facing compile contract**

Update `StrategyPauseMenu.cpp` first to call these not-yet-existing methods from its Continue, Exit, and Escape handlers:

```cpp
Controller->ClosePauseMenu();
Controller->QuitFromPauseMenu();
```

- [x] **Step 2: Build once to verify RED**

Run the editor build.

Expected: compile failure because the controller methods are undeclared.

- [x] **Step 3: Add controller menu lifecycle**

Add to the controller header:

```cpp
class UStrategyPauseMenu;

UPROPERTY(Transient)
TObjectPtr<UStrategyPauseMenu> PauseMenu;

public:
	void OpenPauseMenu();
	void ClosePauseMenu();
	void QuitFromPauseMenu();
	bool IsPauseMenuOpen() const;
```

In `BeginPlay()`, create one native menu for the local controller, initialize it, add it to the player screen above the existing HUD, and start it collapsed.

`OpenPauseMenu()` must cancel building/wall placement, clear wall previews and drag-selection visuals, call `SetPause(true)`, show the pause page, set keyboard focus to the menu, keep `bShowMouseCursor = true`, and apply `FInputModeUIOnly`.

`ClosePauseMenu()` collapses the menu, calls `SetPause(false)`, keeps the cursor visible, releases Slate pointer capture, and applies `FInputModeGameAndUI` without hiding or locking the cursor. This changes only world pause, not `AStrategyGameState` match completion. The visible-cursor mode reflects the packaged regression fix after `FInputModeGameOnly` caused left and right mouse buttons to remain unavailable after clicking Continue.

`HandlePauseKey()` opens the menu instead of toggling pause directly. While UIOnly is active, the focused widget handles subsequent Escape presses. `QuitFromPauseMenu()` uses the existing `UKismetSystemLibrary::QuitGame` call without requiring match completion.

- [x] **Step 4: Build and run both targeted settings tests**

Run the editor build, then:

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests RTS.Strategy.Systems.Settings' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/ue project/RTS/Saved/Verification/M4/SettingsTargeted' '-abslog=D:/ue project/RTS/Saved/Verification/M4/SettingsTargeted.log' -nosplash
```

Expected: `SettingsRules` and `SettingsMenuFlow` both succeed, `0` failed.

---

### Task 4: Register defaults and verify packaged persistence flow

**Files:**
- Modify: `Config/DefaultEngine.ini`
- Create: `docs/M4-SETTINGS-CURRENT.md`
- Modify: `docs/ROADMAP.md`
- Create: `docs/verification/M4-settings-tests.json`
- Create: `docs/verification/M4-settings-smoke.json`

**Interfaces:**
- Consumes: `/Script/RTS.StrategyGameUserSettings` and completed controller/menu integration.
- Produces: the `Windows_M4_Settings` candidate and M4-02 handoff evidence.

- [x] **Step 1: Register the settings class**

Under the existing `[/Script/Engine.Engine]` section in `DefaultEngine.ini`, add exactly:

```ini
GameUserSettingsClassName=/Script/RTS.StrategyGameUserSettings
```

- [x] **Step 2: Rebuild and run the targeted tests once**

Run the Task 3 build and targeted test commands. Copy the resulting `index.json` to `docs/verification/M4-settings-tests.json` and verify the JSON reports `2` succeeded and `0` failed.

- [x] **Step 3: Package the M4-02 candidate**

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun '-project=D:/ue project/RTS/RTS.uproject' -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive '-archivedirectory=D:/ue project/RTS/Builds/Windows_M4_Settings' -utf8output
```

Expected: `BUILD SUCCESSFUL` and `Builds/Windows_M4_Settings/RTS.exe` exists with a current timestamp.

- [x] **Step 4: Run a non-interactive startup smoke check**

Start the packaged executable hidden, let it remain alive for 20 seconds, then close it using the existing M4 smoke-check procedure. Record process lifetime and scan the fresh packaged log for `Fatal error`, `Unhandled Exception`, `Assertion failed`, `LowLevelFatalError`, `Couldn't find file`, and `Failed to load`. Write counts and paths to `docs/verification/M4-settings-smoke.json`.

Expected: the process remains alive until the scripted close and all six keyword counts are zero. This check does not claim that menu controls or persistence were clicked.

Observed: the process remained alive for 20.08 seconds. The five raw `Failed to load` matches are four optional profiling DLLs and the unconfigured default `SoundConcurrencyObject` notice; classified project loading failures and the other five failure keywords are zero. The raw and classified counts are both retained in the evidence file.

- [x] **Step 5: Write the manual handoff and update the roadmap**

Create `docs/M4-SETTINGS-CURRENT.md` with candidate path and this exact manual checklist:

```text
1. 按 Esc：世界暂停，暂停页可点击，战场不能接收命令。
2. 打开设置：将音量改为 35%，窗口模式和分辨率各切换一次，画质改为“中”。
3. 点击“应用并返回”，关闭程序并重新启动，确认四项值保留。
4. 再次修改四项后点击“放弃并返回”，确认已保存值未改变。
5. 在暂停页按 Esc 和点击“继续游戏”各测试一次，确认均能恢复战局。
```

Keep M4-02 as `待验收` until the user completes this packaged manual check. Add one roadmap execution note summarizing targeted test counts, build/package result, smoke result, and candidate path. Do not mark M4-03 started.

- [x] **Step 6: Final consistency check**

Run:

```powershell
git diff --check
rg -n "GameUserSettingsClassName|MasterVolume|OpenPauseMenu|FInputModeUIOnly|Windows_M4_Settings" Config Source docs
```

Expected: no whitespace errors; every runtime interface, config registration, candidate reference, and verification document is present. Existing line-ending conversion warnings may be reported separately and are not whitespace errors.
