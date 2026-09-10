# M5 Town Specialization and Supply Network Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deepen the existing skirmish loop with player-chosen town specializations, a soft supply network, capture persistence, downgrade refunds, readable HUD controls, and fair AI use.

**Architecture:** Pure rules in `StrategySystems` own state transitions, numerical effects, graph connectivity, and AI choices. `AStrategyControlPoint` owns per-town runtime state, while `AStrategyGameState` derives faction economy and training modifiers from control points and completed buildings; controller, native UMG, and canvas HUD provide interaction and presentation.

**Tech Stack:** Unreal Engine 5.8 C++, existing Strategy module, UMG/Slate, Canvas HUD, Unreal Automation Tests, Windows BuildCookRun.

**Spec:** `docs/superpowers/specs/2026-09-07-city-state-rts-territory-specialization-design.md`

## Global Constraints

- All files and text use UTF-8; identifiers are English and new comments are Chinese.
- Keep the existing single-player Windows rules, three unit types, building system, fog of war, two maps, and capital-destruction victory condition.
- Do not add workers, resource nodes, technology trees, multi-level specializations, convoys, territory-score victory, networking, plugins, or third-party dependencies.
- Preserve existing comments and TODO markers unless a directly affected statement must be corrected.
- Player and AI call the same spend, specialization, downgrade, reactivation, supply, and combat interfaces.
- Keep the mouse visible in normal RTS play and preserve the verified `GameAndUI` resume behavior.
- Use targeted tests and one editor build per reviewed task; do not run the full gameplay suite unless a targeted failure indicates a wider regression.
- Package only after M5-03 is integrated; do not create intermediate M5-01 or M5-02 Windows packages.
- The checkout is shared and substantially dirty. Preserve unrelated changes and do not commit, reset, clean, or create a worktree from this checkout.
- After a new candidate starts successfully, retain only the five newest `Builds/Windows_*` directories using verified absolute paths. Never include source, content, configuration, saves, documents, evidence, original assets, or user saves in cleanup.

---

### Task 1: M5-01 pure town-development and supply rules

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyTypes.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Create: `Source/RTS/Variant_Strategy/Tests/StrategyTerritoryTests.cpp`

**Interfaces:**
- Consumes: `EStrategyFaction`, `FVector2D`, and existing automation-test conventions.
- Produces: `EStrategyTownSpecialization`, `EStrategyTownDevelopmentState`, `FStrategyTownDevelopment`, `FStrategySupplyNode`, `FStrategyTownDevelopmentRules`, `FStrategySupplyRules`, and `FStrategyTownSpecializationRules`.

- [x] **Step 1: Add the failing development-state tests**

Create three tests under the `RTS.Strategy.Territory` prefix:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyTownDevelopmentTest,
	"RTS.Strategy.Territory.DevelopmentRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyTownDevelopmentTest::RunTest(const FString& Parameters)
{
	FStrategyTownDevelopment Town;
	TestFalse(TEXT("249 金币不能开始专精"),
		FStrategyTownDevelopmentRules::CanStartSpecialization(Town, EStrategyTownSpecialization::Trade, 249.0f));
	TestTrue(TEXT("250 金币可以开始专精"),
		FStrategyTownDevelopmentRules::CanStartSpecialization(Town, EStrategyTownSpecialization::Trade, 250.0f));

	FStrategyTownDevelopmentRules::StartSpecialization(Town, EStrategyTownSpecialization::Trade);
	TestEqual(TEXT("开始后进入建设中"), Town.State, EStrategyTownDevelopmentState::Building);
	FStrategyTownDevelopmentRules::Update(Town, 19.0f, false, false);
	TestEqual(TEXT("十九秒尚未完成"), Town.State, EStrategyTownDevelopmentState::Building);
	FStrategyTownDevelopmentRules::Update(Town, 2.0f, true, false);
	TestEqual(TEXT("争夺时暂停"), Town.ProgressSeconds, 19.0f);
	FStrategyTownDevelopmentRules::Update(Town, 1.0f, false, false);
	TestEqual(TEXT("二十秒完成"), Town.State, EStrategyTownDevelopmentState::Active);

	FStrategyTownDevelopmentRules::StartDowngrade(Town);
	TestEqual(TEXT("降级立即停用"), Town.State, EStrategyTownDevelopmentState::Downgrading);
	const EStrategyTownUpdateResult DowngradeResult = FStrategyTownDevelopmentRules::Update(Town, 10.0f, false, false);
	TestEqual(TEXT("降级完成事件"), DowngradeResult, EStrategyTownUpdateResult::DowngradeCompleted);
	TestEqual(TEXT("降级后未专精"), Town.Specialization, EStrategyTownSpecialization::None);
	TestEqual(TEXT("退款为四成"), FStrategyTownDevelopmentRules::GetDowngradeRefund(), 100.0f);
	return true;
}
```

Add a second lifecycle case that starts from `Active/Fortress`, calls `HandleOwnershipChanged`, expects `DisabledAfterCapture`, verifies contested reactivation does not progress, verifies five seconds without an owner squad returns progress to zero, and verifies ten continuous seconds with an owner squad returns `ReactivationCompleted` and `Active`.

- [x] **Step 2: Add failing supply and effect tests**

Use literal node layouts for both maps:

```cpp
TArray<FStrategySupplyNode> Plains = {
	{FVector2D(-7000.0f, 0.0f), EStrategyFaction::Player, true},
	{FVector2D(0.0f, 0.0f), EStrategyFaction::Player, false},
	{FVector2D(0.0f, 4000.0f), EStrategyFaction::Player, false},
	{FVector2D(0.0f, -4000.0f), EStrategyFaction::Enemy, false}
};
const TSet<int32> Connected = FStrategySupplyRules::FindConnectedTownIndices(Plains, EStrategyFaction::Player);
TestTrue(TEXT("中央城镇连接主城"), Connected.Contains(1));
TestTrue(TEXT("同阵营翼城通过中央城镇连接"), Connected.Contains(2));
TestFalse(TEXT("敌方城镇不连接"), Connected.Contains(3));
```

Add a River Valley fixture using `(-7000,0)`, `(-2200,3900)`, `(0,0)`, and `(2200,-3900)`. Verify all-player ownership connects every town, then change the center town to Enemy and verify the near town remains connected while the far town disconnects. Add literal effect assertions:

```cpp
TestEqual(TEXT("贸易主体收入"), FStrategyTownSpecializationRules::GetIncomeBonus(
	EStrategyTownSpecialization::Trade, EStrategyTownDevelopmentState::Active, false), 4.0f);
TestEqual(TEXT("贸易连接收入"), FStrategyTownSpecializationRules::GetIncomeBonus(
	EStrategyTownSpecialization::Trade, EStrategyTownDevelopmentState::Active, true), 6.0f);
TestEqual(TEXT("征募主体倍率"), FStrategyTownSpecializationRules::GetTrainingTimeMultiplier(
	EStrategyTownSpecialization::Recruitment, EStrategyTownDevelopmentState::Active, false), 0.8f);
TestEqual(TEXT("征募连接倍率"), FStrategyTownSpecializationRules::GetTrainingTimeMultiplier(
	EStrategyTownSpecialization::Recruitment, EStrategyTownDevelopmentState::Active, true), 0.7f);
TestEqual(TEXT("要塞占领秒数"), FStrategyTownSpecializationRules::GetCaptureDuration(
	EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::Active), 15.0f);
```

Also assert that `Building`, `Downgrading`, and `DisabledAfterCapture` return no specialization bonuses.

- [x] **Step 3: Build once to verify RED**

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\Build.bat' RTSEditor Win64 Development '-Project=D:/ue project/RTS/RTS.uproject' -WaitMutex -NoHotReloadFromIDE -utf8output
```

Expected: compile failure because the new enums and rule types do not exist.

- [x] **Step 4: Implement the minimal pure model**

Add these enums to `StrategyTypes.h`:

```cpp
UENUM(BlueprintType)
enum class EStrategyTownSpecialization : uint8 { None, Trade, Recruitment, Fortress };

UENUM(BlueprintType)
enum class EStrategyTownDevelopmentState : uint8
{
	Unspecialized,
	Building,
	Active,
	Downgrading,
	DisabledAfterCapture
};
```

Add these plain types and exact public functions to `StrategySystems.h`:

```cpp
enum class EStrategyTownUpdateResult : uint8
{
	None,
	BuildCompleted,
	DowngradeCompleted,
	ReactivationCompleted
};

struct FStrategyTownDevelopment
{
	EStrategyTownSpecialization Specialization = EStrategyTownSpecialization::None;
	EStrategyTownDevelopmentState State = EStrategyTownDevelopmentState::Unspecialized;
	float ProgressSeconds = 0.0f;
};

struct FStrategySupplyNode
{
	FVector2D Location = FVector2D::ZeroVector;
	EStrategyFaction Owner = EStrategyFaction::Neutral;
	bool bCapital = false;
};

struct FStrategyTownDevelopmentRules
{
	static constexpr float SpecializationCost = 250.0f;
	static constexpr float BuildDuration = 20.0f;
	static constexpr float DowngradeDuration = 10.0f;
	static constexpr float ReactivationDuration = 10.0f;
	static constexpr float ReactivationRecoveryDuration = 5.0f;
	static bool CanStartSpecialization(const FStrategyTownDevelopment& Town,
		EStrategyTownSpecialization Specialization, float Gold);
	static bool CanStartDowngrade(const FStrategyTownDevelopment& Town);
	static void StartSpecialization(FStrategyTownDevelopment& Town, EStrategyTownSpecialization Specialization);
	static void StartDowngrade(FStrategyTownDevelopment& Town);
	static void HandleOwnershipChanged(FStrategyTownDevelopment& Town);
	static EStrategyTownUpdateResult Update(FStrategyTownDevelopment& Town, float DeltaSeconds,
		bool bContested, bool bOwnerSquadPresent);
	static float GetDowngradeRefund();
};

struct FStrategySupplyRules
{
	static constexpr float LinkDistance = 7500.0f;
	static TSet<int32> FindConnectedTownIndices(const TArray<FStrategySupplyNode>& Nodes,
		EStrategyFaction Faction);
};

struct FStrategyTownSpecializationRules
{
	static float GetIncomeBonus(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State, bool bConnected);
	static int32 GetPopulationBonus(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State);
	static float GetTrainingTimeMultiplier(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State, bool bConnected);
	static float GetCaptureDuration(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State);
	static float GetFortressRange(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State, bool bConnected);
	static float GetFortressDamage(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State, bool bConnected);
};
```

Implement linear state transitions. `Building` and `Downgrading` accumulate only while not contested. `DisabledAfterCapture` accumulates only while the owner's squad is present and the point is uncontested; otherwise it loses `DeltaSeconds * 2.0f`, which clears ten seconds of progress in five seconds. Use breadth-first traversal over same-owner nodes with two-dimensional squared distance at or below `7500²`.

`HandleOwnershipChanged` must use the previous state explicitly: `Building` clears the unfinished specialization and returns to `Unspecialized`; `Active` and `Downgrading` preserve the completed specialization and enter `DisabledAfterCapture`; an already disabled specialization remains disabled; `Unspecialized` remains unchanged. Ownership change never grants a refund.

- [x] **Step 5: Build and run the grouped M5-01 GREEN tests**

Run the editor build from Step 3, then:

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests RTS.Strategy.Territory' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/ue project/RTS/Saved/Verification/M5/TerritoryRules' '-abslog=D:/ue project/RTS/Saved/Verification/M5/TerritoryRules.log' -nosplash
```

Expected: all territory rule tests succeed with zero warnings and failures.

- [x] **Step 6: Review M5-01 pure rules**

Review only the files in this task. Confirm the state graph has no path that grants an active bonus during building, downgrade, or capture paralysis; confirm a severed supply node affects only the connected bonus; do not package.

---

### Task 2: M5-01 runtime economy, training, capture, and fortress integration

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyTypes.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyTerritoryTests.cpp`

**Interfaces:**
- Consumes: Task 1 town state and specialization rules, existing control-point registry, training queue, tower targeting, fog visibility, and faction economy.
- Produces: `FStrategyTownContribution`, `FStrategyFactionEconomyRules::Calculate`, `AStrategyGameState::TryStartTownSpecialization`, `TryStartTownDowngrade`, `NotifyTownDevelopmentChanged`, `IsTownSupplyConnected`, `GetTrainingTimeMultiplierAt`, and runtime town getters.

- [x] **Step 1: Add the failing derived-economy test**

Add:

```cpp
TArray<FStrategyTownContribution> Contributions = {
	{EStrategyFaction::Player, 5.0f, 20, false},
	{EStrategyFaction::Player, 3.0f, 5, false},
	{EStrategyFaction::Player, 7.0f, 15, true},
	{EStrategyFaction::Enemy, 9.0f, 5, true}
};
const FStrategyFactionEconomyTotals Totals = FStrategyFactionEconomyRules::Calculate(
	EStrategyFaction::Player, Contributions, 10);
TestEqual(TEXT("只汇总己方据点"), Totals.OwnedPoints, 3);
TestEqual(TEXT("基础与专精收入汇总"), Totals.IncomePerSecond, 15.0f);
TestEqual(TEXT("据点与一座民居人口汇总"), Totals.PopulationCap, 50);
```

The final integer is completed-building population. Add a case whose raw cap exceeds `60` and assert the returned cap is `60`.

- [x] **Step 2: Build once to verify RED**

Run the Task 1 editor build command.

Expected: compile failure because economy contribution types and calculator do not exist.

- [x] **Step 3: Implement derived economy and public town actions**

Add:

```cpp
struct FStrategyTownContribution
{
	EStrategyFaction Owner = EStrategyFaction::Neutral;
	float IncomePerSecond = 0.0f;
	int32 PopulationCap = 0;
};

struct FStrategyFactionEconomyTotals
{
	float IncomePerSecond = 0.0f;
	int32 PopulationCap = 0;
	int32 OwnedPoints = 0;
};

struct FStrategyFactionEconomyRules
{
	static FStrategyFactionEconomyTotals Calculate(EStrategyFaction Faction,
		const TArray<FStrategyTownContribution>& Contributions, int32 CompletedBuildingPopulation);
};
```

Add these methods to `AStrategyGameState`:

```cpp
bool TryStartTownSpecialization(EStrategyFaction Faction, AStrategyControlPoint* Town,
	EStrategyTownSpecialization Specialization);
bool TryStartTownDowngrade(EStrategyFaction Faction, AStrategyControlPoint* Town);
void NotifyTownDevelopmentChanged(AStrategyControlPoint* Town);
void RecalculateFactionEconomy();
bool IsTownSupplyConnected(const AStrategyControlPoint* Town) const;
float GetTrainingTimeMultiplierAt(EStrategyFaction Faction, const FVector& Location) const;
```

`TryStartTownSpecialization` validates ownership, non-capital status and pure-rule eligibility, then calls existing `TrySpendAndReserve(Faction, 250.0f, 0)` before starting the town. `TryStartTownDowngrade` validates the same ownership and calls the pure rule without spending. `RecalculateFactionEconomy` rebuilds income, cap and owned points from all control points plus completed building population; it replaces incremental point contribution updates. Call it when points register or change owner, when town development changes, and when population buildings complete or are removed.

- [x] **Step 4: Wire runtime state into control points and training**

Add public control-point methods/getters:

```cpp
bool StartSpecialization(EStrategyTownSpecialization Specialization);
bool StartDowngrade();
const FStrategyTownDevelopment& GetTownDevelopment() const { return TownDevelopment; }
float GetDevelopmentProgress() const;
float GetRequiredCaptureDuration() const;
bool IsContested() const { return bWasContested; }
```

`AStrategyControlPoint::Tick` passes current contested and owner-squad presence into the pure update. On `DowngradeCompleted`, add `100` gold to the current owner. On any completion event, call `NotifyTownDevelopmentChanged`. On ownership change, cancel unfinished construction; preserve completed or downgrading specialization as `DisabledAfterCapture`; then notify the game state.

Change `FStrategyCaptureState::Update` to accept `float CaptureDurationSeconds = 10.0f`; compare against this argument rather than a hard-coded value. Divide HUD capture progress by `GetRequiredCaptureDuration()`.

In `AStrategyBuilding::QueueUnit`, enqueue `UnitDefinition->TrainingTime * State->GetTrainingTimeMultiplierAt(Faction, GetActorLocation())`. Overlapping recruitment territories use the lowest multiplier.

Add `AStrategyControlPoint::UpdateFortress(float DeltaSeconds)`, modeled on the existing tower code. Use the tower building definition's projectile mesh and attack sound, `FStrategyTowerTargetRules`, current fog visibility, nearest valid enemy unit, and the specialization rule's range/damage. Non-active fortress states never attack.

- [x] **Step 5: Run one grouped verification for all M5-01 work**

Run the editor build and the single `RTS.Strategy.Territory` automation command. Expected: all tests pass, zero failures. Do not package.

- [x] **Step 6: Update M5-01 evidence and review checkpoint**

Copy the fresh report to `docs/verification/M5-territory-rules.json`. Update `docs/ROADMAP.md` so M5-01 becomes `待验收`, recording test count and build result. Review runtime ownership transitions, house population recomputation, queue-time training snapshots, and fortress fog checks before proceeding.

---

### Task 3: M5-02 native town-management panel and selection

**Files:**
- Create: `Source/RTS/Variant_Strategy/UI/StrategyTownPanel.h`
- Create: `Source/RTS/Variant_Strategy/UI/StrategyTownPanel.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyTerritoryTests.cpp`

**Interfaces:**
- Consumes: game-state town action methods and control-point getters from Task 2.
- Produces: `UStrategyTownPanel`, `AStrategyPlayerController::SelectControlPoint`, `GetSelectedControlPoint`, `TrySpecializeSelectedTown`, and `TryDowngradeSelectedTown`.

- [x] **Step 1: Add the failing action-availability test**

Add `FStrategyTownActionRules` to the test before production code exists and assert:

```cpp
TestTrue(TEXT("己方未专精城镇可管理"), FStrategyTownActionRules::CanChooseSpecialization(
	EStrategyFaction::Player, EStrategyFaction::Player, false, EStrategyTownDevelopmentState::Unspecialized));
TestFalse(TEXT("敌方城镇不可管理"), FStrategyTownActionRules::CanChooseSpecialization(
	EStrategyFaction::Player, EStrategyFaction::Enemy, false, EStrategyTownDevelopmentState::Unspecialized));
TestFalse(TEXT("主城不可专精"), FStrategyTownActionRules::CanChooseSpecialization(
	EStrategyFaction::Player, EStrategyFaction::Player, true, EStrategyTownDevelopmentState::Unspecialized));
TestTrue(TEXT("己方激活城镇可降级"), FStrategyTownActionRules::CanDowngrade(
	EStrategyFaction::Player, EStrategyFaction::Player, false, EStrategyTownDevelopmentState::Active));
```

- [x] **Step 2: Build once to verify RED, then implement the rule**

Run the editor build and verify the missing rule failure. Add this exact public interface to `StrategySystems.h` and implement both functions as direct comparisons in `StrategySystems.cpp`, then rebuild without running the complete automation group yet:

```cpp
struct FStrategyTownActionRules
{
	static bool CanChooseSpecialization(EStrategyFaction Viewer, EStrategyFaction Owner,
		bool bCapital, EStrategyTownDevelopmentState State);
	static bool CanDowngrade(EStrategyFaction Viewer, EStrategyFaction Owner,
		bool bCapital, EStrategyTownDevelopmentState State);
};
```

- [x] **Step 3: Implement the native UMG panel**

Create `UStrategyTownPanel : UUserWidget` with:

```cpp
void InitializeForController(AStrategyPlayerController* InController);
void ShowTown(AStrategyControlPoint* InTown);
void HideTown();
void Refresh();
```

Build a bottom-centered `UBorder` and `UVerticalBox` in `RebuildWidget()`. Show owner, base `3 金币/秒 + 5 人口`, specialization, supply state, effective bonuses, state/progress, three specialization buttons, and one downgrade button. The downgrade label must include `返还 100 金币`. Button delegates call controller methods; failed actions play the existing invalid-action feedback and leave the panel open.

Refresh from `NativeTick` only while visible. Enemy or neutral towns show public owner and completed specialization only when currently visible; never show operation buttons, live progress, or enemy supply state. Panel button delegates call the public controller methods; those controller methods call the existing invalid-action feedback when the game-state action returns false.

- [x] **Step 4: Integrate control-point selection**

In local `BeginPlay`, create one panel, initialize it, add it to the player screen below the pause menu, and start collapsed. Apply the same visible-cursor `FInputModeGameAndUI` configuration used by `ClosePauseMenu` so native buttons and battlefield controls coexist from initial startup.

Add:

```cpp
void SelectControlPoint(AStrategyControlPoint* Point);
AStrategyControlPoint* GetSelectedControlPoint() const { return SelectedControlPoint; }
bool TrySpecializeSelectedTown(EStrategyTownSpecialization Specialization);
bool TryDowngradeSelectedTown();
```

Handle `AStrategyControlPoint` in `SelectClick` before the ground fallback. Selecting any control point clears units and buildings; only an owned non-capital exposes actions. Selecting a squad, building, or ground clears the town selection. Keep right-click orders unchanged.

- [x] **Step 5: Run the grouped M5-02 code verification**

Run one editor build and the grouped `RTS.Strategy.Territory` automation command. Expected: all territory tests pass with zero failures. Do not package.

- [ ] **Step 6: M5-02 panel review checkpoint**

In PIE, check one owned, neutral, and enemy town. Confirm the panel does not block WASD, wheel, ordinary selection, right-click orders, or pause-menu input; confirm buttons spend once and invalid actions give one feedback. Record observations in `docs/M5-CURRENT.md` without marking M5-02 complete yet.

---

### Task 4: M5-02 world specialization feedback and supply lines

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.h`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyTerritoryTests.cpp`

**Interfaces:**
- Consumes: selected town, supply state, town development state, projection, canvas drawing, and fog visibility.
- Produces: `FStrategyTownVisibilityRules`, projected town icons/progress, and player-only supply lines.

- [x] **Step 1: Add the failing visibility-policy test**

Assert literal policy results:

```cpp
TestTrue(TEXT("己方城镇始终显示管理详情"), FStrategyTownVisibilityRules::CanShowLiveDetails(
	EStrategyFaction::Player, EStrategyFaction::Player, false));
TestTrue(TEXT("可见敌镇显示公开详情"), FStrategyTownVisibilityRules::CanShowPublicDetails(
	EStrategyFaction::Player, EStrategyFaction::Enemy, true));
TestFalse(TEXT("迷雾外敌镇不显示实时详情"), FStrategyTownVisibilityRules::CanShowLiveDetails(
	EStrategyFaction::Player, EStrategyFaction::Enemy, false));
TestFalse(TEXT("绝不显示敌方补给"), FStrategyTownVisibilityRules::CanShowSupplyConnection(
	EStrategyFaction::Player, EStrategyFaction::Enemy));
```

- [x] **Step 2: Build once to verify RED, then implement the policy**

Run the editor build, verify the missing-rule failure, add this exact public interface to `StrategySystems.h`, implement the three direct boolean functions in `StrategySystems.cpp`, and rebuild:

```cpp
struct FStrategyTownVisibilityRules
{
	static bool CanShowLiveDetails(EStrategyFaction Viewer, EStrategyFaction Owner,
		bool bCurrentlyVisible);
	static bool CanShowPublicDetails(EStrategyFaction Viewer, EStrategyFaction Owner,
		bool bCurrentlyVisible);
	static bool CanShowSupplyConnection(EStrategyFaction Viewer, EStrategyFaction Owner);
};
```

- [x] **Step 3: Draw projected town state**

Add HUD helpers that draw a compact screen-space symbol above visible non-capital towns: coin for Trade, chevrons for Recruitment, shield for Fortress, and a neutral circle for None. Reuse lines, rectangles, circles and existing faction colors; do not add textures.

Below the icon, draw a narrow progress bar for Building, Downgrading, or DisabledAfterCapture only when `CanShowLiveDetails` permits it. Draw a gray broken-link mark on disconnected owned specialized towns.

- [x] **Step 4: Draw player supply links**

For every pair of player-owned control points at or below `7500 cm`, draw a thin line between their projected world positions. Use the player faction color when both endpoints are reachable from the capital and muted gray otherwise. Do not draw enemy links or infer enemy connection through fog.

- [ ] **Step 5: Run one grouped verification and review M5-02**

Run the editor build and grouped territory tests once. In PIE, verify icons, live progress, connection changes after capture, and no fog leakage on both maps. Update `docs/M5-CURRENT.md` and mark M5-02 `待验收`; do not package.

---

### Task 5: M5-03 fair AI town development

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategyTerritoryTests.cpp`

**Interfaces:**
- Consumes: common game-state actions, owned points, visible threats, fixed capital positions, and town development state.
- Produces: `FStrategyTownAIInputs`, `FStrategyTownAIPlanner::ChooseSpecialization`, reactivation orders, and bounded downgrade behavior.

- [x] **Step 1: Add the failing AI-choice test**

Define inputs and assert:

```cpp
FStrategyTownAIInputs Inputs;
Inputs.bHasTradeTown = false;
Inputs.bHasRecruitmentTown = false;
Inputs.bTownThreatened = false;
TestEqual(TEXT("第一座优先贸易"), FStrategyTownAIPlanner::ChooseSpecialization(Inputs),
	EStrategyTownSpecialization::Trade);

Inputs.bHasTradeTown = true;
TestEqual(TEXT("第二座优先征募"), FStrategyTownAIPlanner::ChooseSpecialization(Inputs),
	EStrategyTownSpecialization::Recruitment);

Inputs.bHasRecruitmentTown = true;
TestEqual(TEXT("已有经济与征募后选择要塞"), FStrategyTownAIPlanner::ChooseSpecialization(Inputs),
	EStrategyTownSpecialization::Fortress);

Inputs.bTownThreatened = true;
TestEqual(TEXT("受威胁城镇优先要塞"), FStrategyTownAIPlanner::ChooseSpecialization(Inputs),
	EStrategyTownSpecialization::Fortress);
```

Also test `ShouldRespecialize(Current, Preferred, Gold)` is false below `500`, false when equal, and true only when different at or above `500`.

- [x] **Step 2: Build once to verify RED, then implement the AI rule**

Run the editor build, confirm the missing-type failure, add this exact public model to `StrategySystems.h`, implement the pure planner with the exact priority from Step 1 in `StrategySystems.cpp`, then rebuild:

```cpp
struct FStrategyTownAIInputs
{
	bool bHasTradeTown = false;
	bool bHasRecruitmentTown = false;
	bool bTownThreatened = false;
};

struct FStrategyTownAIPlanner
{
	static EStrategyTownSpecialization ChooseSpecialization(const FStrategyTownAIInputs& Inputs);
	static bool ShouldRespecialize(EStrategyTownSpecialization Current,
		EStrategyTownSpecialization Preferred, float Gold);
};
```

- [x] **Step 3: Integrate reactivation and specialization priorities**

At the start of `RunDecision`, after immediate defense:

1. Find an enemy-owned `DisabledAfterCapture` town and issue one squad an `AttackMove` order to its center until reactivation completes.
2. If no town engineering project exists and enemy gold is at least `250`, select one stable unspecialized town and call `TryStartTownSpecialization` with the pure planner's result.
3. If gold is at least `500`, no project exists, and an inherited active specialization differs from the preferred role, call `TryStartTownDowngrade`.
4. Otherwise continue existing neutral capture, facility building, training, and attack behavior.

Threatened means a currently visible player squad within the existing `2200 cm` threat radius. Never inspect hidden player units. Count preserved and active specializations when deciding whether Trade or Recruitment already exists, so capture paralysis does not trigger duplicate long-term choices.

- [x] **Step 4: Run the final targeted M5 automation group**

Run one editor build and the grouped `RTS.Strategy.Territory` command. Copy the report to `docs/verification/M5-territory-tests.json`. Expected: all territory tests pass, zero warnings and failures.

- [ ] **Step 5: Run a five-minute AI PIE observation**

用户于 2026-09-08 取消完整五分钟观察。已保留约 77 秒有效日志：AI 完成一次占领并开始贸易专精；未将未观察行为记为通过。

Observe one map for five minutes. Record, without claiming more than observed: AI income, whether it captures a town, its first two specialization choices, whether it uses ordinary build/train/attack behavior, and whether any action used hidden player information. If AI cannot reach at least one specialization while continuing normal play, keep M5-03 in progress and diagnose before packaging.

---

### Task 6: M5 candidate, evidence, handoff, and five-version retention

**Files:**
- Modify: `docs/M5-CURRENT.md`
- Modify: `docs/ROADMAP.md`
- Create: `docs/verification/M5-smoke.json`
- Create: `docs/verification/M5-package.csv`
- Create: `docs/verification/M5-build-retention.json`

**Interfaces:**
- Consumes: integrated M5 implementation and fresh targeted-test evidence.
- Produces: `Builds/Windows_M5_Territory`, package manifest, smoke evidence, retention evidence, and manual acceptance checklist.

- [x] **Step 1: Package the single M5 candidate**

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun '-project=D:/ue project/RTS/RTS.uproject' -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive '-archivedirectory=D:/ue project/RTS/Builds/Windows_M5_Territory' -utf8output
```

Expected: `BUILD SUCCESSFUL` and `D:\ue project\RTS\Builds\Windows_M5_Territory\RTS.exe` exists.

- [x] **Step 2: Run one 20-second packaged smoke check**

Launch the packaged executable hidden with a fresh absolute log, keep it alive for 20 seconds, then stop only the exact process returned by `Start-Process -PassThru`. Record lifetime and raw counts for `Fatal error`, `Unhandled Exception`, `Assertion failed`, `LowLevelFatalError`, `Couldn't find file`, and `Failed to load`. Classify known optional profiler DLL and default `SoundConcurrencyObject` messages separately from project failures in `docs/verification/M5-smoke.json`.

- [x] **Step 3: Generate the package hash manifest**

Recursively enumerate files under `Builds/Windows_M5_Territory`, sort by relative path, and write UTF-8 CSV rows containing relative path, byte length, and SHA-256 to `docs/verification/M5-package.csv`. Verify the launcher and packaged game executable both appear.

- [x] **Step 4: Prune only superseded Windows candidate directories**

First perform a read-only inventory:

```powershell
$buildRoot = (Resolve-Path -LiteralPath 'D:\ue project\RTS\Builds').Path
$candidates = Get-ChildItem -LiteralPath $buildRoot -Directory |
	Where-Object { $_.Name -like 'Windows_*' } |
	Sort-Object LastWriteTime -Descending
$keep = @($candidates | Select-Object -First 5)
$remove = @($candidates | Select-Object -Skip 5)
$keep | Select-Object FullName, LastWriteTime
$remove | Select-Object FullName, LastWriteTime
```

Resolve every removal target again and verify `Split-Path -Parent` equals the exact `$buildRoot`, its leaf starts with `Windows_`, and it is not `Windows_M5_Territory`. Only after all targets pass, remove each explicit target with `Remove-Item -LiteralPath <verified absolute path> -Recurse`. Write kept and removed paths to `docs/verification/M5-build-retention.json`, including whether removed directories are recoverable. Never build a removal command from a wildcard or unresolved variable.

- [x] **Step 5: Write the manual M5 acceptance handoff**

Update `docs/M5-CURRENT.md` with candidate path, test/build/package/smoke evidence, raw versus classified load messages, known limitations, and this checklist:

```text
1. 在两张地图分别占领一座城镇，确认可以选择贸易、征募或要塞专精。
2. 建设期间制造争夺，确认进度暂停；完成后确认专精主体效果。
3. 占领中继城镇建立连接，再失去它切断远端城镇，确认只失去补给加成。
4. 攻占一个已有专精的敌方城镇，确认专精保留但瘫痪，驻军 10 秒后重新启用。
5. 降级一座城镇，确认完成后返还 100 金币，并可重新支付 250 金币选择新专精。
6. 比较征募城领地内外的新训练队列；确认要塞射击、15 秒占领和迷雾信息限制。
7. 观察 AI 完成占领、至少两种专精、生产、进攻，以及一次城镇防守或重新启用。
8. 两张地图各完成一整局，确认胜负和 R 重开仍然正常。
```

Set M5-01 and M5-02 to complete only after their reviewed evidence exists. Keep M5-03 as `待验收` until the user completes the packaged checklist. Do not start M6-01.

- [x] **Step 6: Run the final consistency check**

Run:

```powershell
git diff --check
rg -n "TownSpecialization|DisabledAfterCapture|FindConnectedTownIndices|TryStartTownSpecialization|Windows_M5_Territory|最近 5 个" Source Config CONTEXT.md docs
```

Expected: no whitespace errors; runtime interfaces, domain terms, candidate, evidence, and retention policy are present. Report existing line-ending conversion warnings separately.
