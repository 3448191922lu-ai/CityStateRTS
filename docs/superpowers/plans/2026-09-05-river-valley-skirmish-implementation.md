# River Valley Skirmish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the `LVL_RiverValleySkirmish` map with three constrained crossings while preserving the original map and all existing RTS rules.

**Architecture:** A small C++ map definition selected from the current level name supplies spawn positions, fog bounds, terrain selection, and no-build zones. `AStrategyMapTerrain` generates the river-valley blocking geometry from engine primitives at runtime, while the two `.umap` assets remain separate entry points that share the same game systems.

**Tech Stack:** Unreal Engine 5.8 C++, UE Automation Tests, runtime navigation, engine basic meshes/material instances, Unreal Editor asset duplication, Windows packaging.

**Spec:** `docs/superpowers/specs/2026-09-05-river-valley-skirmish-design.md`

## Global Constraints

- All files and text use UTF-8; identifiers are English and comments are Chinese.
- Do not download plugins or external assets.
- Keep the implementation compact and local to the existing Strategy module.
- Preserve `LVL_CityStateSkirmish` and all existing gameplay behavior.
- Do not add a map-selection menu or new gameplay rules.
- Because the current branch contains accepted but uncommitted M0-M2 work, do not create commits or move to a clean worktree during this task.
- Reduce test frequency: one RED verification before production code, then one consolidated GREEN verification and one package smoke check after implementation.

---

### Task 1: Map definition and regression tests

**Files:**
- Create: `Source/RTS/Variant_Strategy/StrategyMapDefinition.h`
- Create: `Source/RTS/Variant_Strategy/StrategyMapDefinition.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp`

**Interfaces:**
- Produces: `FStrategySkirmishMapDefinition FStrategyMapDefinitions::Resolve(const FString& MapName)`.
- Produces: `bool FStrategyMapDefinitions::IsBuildingAllowed(const FStrategySkirmishMapDefinition& Definition, const FVector& Location, const FVector2D& FootprintExtent)`.
- The definition exposes `PlayerCapital`, `EnemyCapital`, `NeutralTowns`, `PlayerSquadStart`, `EnemySquadStart`, `FogMin`, `FogMax`, `bSpawnRiverValleyTerrain`, and `NoBuildZones`.

- [x] **Step 1: Write the failing map-selection test**

Add an automation test that resolves both `LVL_CityStateSkirmish` and `UEDPIE_0_LVL_RiverValleySkirmish`, then asserts the original center-line towns remain unchanged and the river-valley definition returns `(-2200, 3900)`, `(0, 0)`, `(2200, -3900)` with `bSpawnRiverValleyTerrain == true`.

- [x] **Step 2: Write the failing no-build test**

Assert that a footprint centered at `(0, 3000)` and `(0, 0)` is rejected by the river-valley definition, while footprints at `(-1800, 3000)` and `(-5000, 0)` are allowed.

- [x] **Step 3: Run one RED compile**

Run:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\Build.bat' RTSEditor Win64 Development 'D:\ue project\RTS\RTS.uproject' -WaitMutex -NoHotReload
```

Expected: compilation fails because `StrategyMapDefinition.h` and `FStrategyMapDefinitions` do not exist.

- [x] **Step 4: Implement the minimal definitions**

Create a plain struct and static resolver. Match the river map with `MapName.Contains(TEXT("LVL_RiverValleySkirmish"))` so PIE prefixes are accepted. Use one axis-aligned `FBox2D(FVector2D(-750, -7000), FVector2D(750, 7000))` no-build zone. Treat a building footprint as an axis-aligned box and reject intersections.

### Task 2: Drive game startup, fog, placement, and restart from the definition

**Files:**
- Modify: `Source/RTS/Variant_Strategy/StrategyGameMode.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`

**Interfaces:**
- Consumes: `FStrategyMapDefinitions::Resolve(GetWorld()->GetMapName())`.
- Consumes: `FStrategyMapDefinitions::IsBuildingAllowed(...)`.
- Produces: current-level restart through `UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetMapName()))` after removing the PIE prefix with `UWorld::RemovePIEPrefix`.

- [x] **Step 1: Replace startup coordinates**

Resolve the current map once in `AStrategyGameMode::BeginPlay()` and spawn capitals, towns, and initial squads from the returned definition.

- [x] **Step 2: Apply map-specific placement restrictions**

At the start of `AStrategyGameState::CanPlaceBuilding`, resolve the current map and return `false` when `IsBuildingAllowed` rejects the footprint. Keep the existing territory, navigation, and overlap checks unchanged.

- [x] **Step 3: Apply map-specific fog bounds**

In `AStrategyFogOfWar::BeginPlay`, initialize both `128 × 96` grids from `FogMin` and `FogMax`. Derive the fog plane scale and center from those bounds so both maps remain covered.

- [x] **Step 4: Restart the current level**

Replace the hard-coded `LVL_CityStateSkirmish` name in `HandleRestartKey` with the current level's package-safe name after removing any PIE prefix.

### Task 3: Runtime river-valley terrain

**Files:**
- Create: `Source/RTS/Variant_Strategy/StrategyMapTerrain.h`
- Create: `Source/RTS/Variant_Strategy/StrategyMapTerrain.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameMode.cpp`

**Interfaces:**
- Produces: `void AStrategyMapTerrain::InitializeRiverValley()`.
- Consumes: `FStrategySkirmishMapDefinition::bSpawnRiverValleyTerrain`.

- [x] **Step 1: Create the terrain actor**

Create a non-ticking actor with a scene root and compact helpers that add cube mesh components with a transform, color, collision setting, and navigation relevance.

- [x] **Step 2: Build the river and crossings**

Add four river blocker bands spanning the north/south edges while leaving openings centered at `Y = 3000`, `0`, and `-3000`. Add visible bridge decks at the north and south openings and a shallow-water visual at the central opening. Use approximately `1800 cm` bridge openings and a `1200 cm` ford opening.

- [x] **Step 3: Add route-shaping obstacles**

Add a small symmetric set of rock and forest blocks away from capitals and control points. They block units and dynamic navigation but do not add gameplay effects.

- [x] **Step 4: Spawn terrain only for the river map**

In `AStrategyGameMode::BeginPlay`, spawn one `AStrategyMapTerrain` and call `InitializeRiverValley()` only when the selected definition requests it.

### Task 4: Create the second map asset and make it the candidate entry point

**Files:**
- Create: `Content/CityStateRTS/Maps/LVL_RiverValleySkirmish.umap`
- Modify: `Config/DefaultEngine.ini`

**Interfaces:**
- Produces: `/Game/CityStateRTS/Maps/LVL_RiverValleySkirmish` as the editor and packaged default map.

- [x] **Step 1: Duplicate the existing map in Unreal Editor**

Use the Content Browser to duplicate `LVL_CityStateSkirmish` in the same folder as `LVL_RiverValleySkirmish`, then save all. Do not copy or rename the binary package outside the editor.

- [x] **Step 2: Update the default maps**

Set both `GameDefaultMap` and `EditorStartupMap` to:

```ini
/Game/CityStateRTS/Maps/LVL_RiverValleySkirmish.LVL_RiverValleySkirmish
```

- [x] **Step 3: Confirm both map packages are discoverable**

Use the editor asset registry or a cook listing to verify both `/Game/CityStateRTS/Maps/LVL_CityStateSkirmish` and `/Game/CityStateRTS/Maps/LVL_RiverValleySkirmish` resolve as worlds.

### Task 5: Consolidated verification and milestone documentation

**Files:**
- Create: `docs/M3-CURRENT.md`
- Modify: `docs/ROADMAP.md`

**Interfaces:**
- Consumes: all deliverables from Tasks 1-4.
- Produces: an M3-01 candidate package and a concise manual acceptance checklist.

- [x] **Step 1: Run one consolidated build and rules test**

Build `RTSEditor Win64 Development`, then run `RTS.Strategy.Systems`. Expected: compilation succeeds and every Strategy Systems test reports success with zero warnings.

- [x] **Step 2: Package the Windows candidate once**

Package to `Builds/Windows_M3_Candidate` using the same Development packaging flow as M2. Expected: `BUILD SUCCESSFUL` and `RTS.exe` exists.

- [x] **Step 3: Perform one startup smoke check**

Launch the packaged executable long enough for dynamic navigation and the opening state to initialize. Expected: no fatal error, assertion, access violation, or spawn collision failure in the current log.

- [x] **Step 4: Record implementation evidence**

Create `docs/M3-CURRENT.md` with the implemented layout, build/test/package results, known limits, and the remaining manual checks: both maps complete a match; bridge/ford routing; river no-build restriction; AI expansion; current-map `R` restart.

- [x] **Step 5: Update roadmap status**

Set M3-01 to `待验收` after automated checks pass. Mark it `完成` only after the user confirms the complete two-map manual checklist.
