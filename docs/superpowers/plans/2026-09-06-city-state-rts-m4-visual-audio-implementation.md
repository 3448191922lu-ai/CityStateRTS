# City-State RTS M4 Visual and Audio Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the prototype geometry with a coherent medieval low-poly presentation and add readable visual/audio feedback without changing any gameplay rule.

**Architecture:** Existing gameplay actors remain authoritative. Unit and building Data Assets hold type-specific presentation references, while one `UStrategyPresentationDataAsset` holds shared faction, command, state, control-point, environment, and audio resources. Controller and world actors trigger presentation directly from existing successful gameplay paths; no event bus or plugin module is added.

**Tech Stack:** Unreal Engine 5.8 C++, Unreal Python asset automation, Skeletal Mesh single-node animation, Static Mesh/HISM environment rendering, Niagara, UE audio attenuation/concurrency, UE Automation Tests, Windows Development packaging.

**Spec:** `docs/superpowers/specs/2026-09-06-city-state-rts-m4-visual-audio-design.md`

## Global Constraints

- All files and text use UTF-8; identifiers are English and comments are Chinese.
- Use UE 5.8 built-in capabilities only; do not install plugins.
- Raw downloads live under `D:\素材\游戏素材\CityStateRTS\M4`; only selected files enter `/Game/CityStateRTS/Art/`.
- Quaternius supplies the low-poly models and animations; Poly Haven supplies selected 2K surfaces; Kenney supplies sound effects. Every imported item must have a recorded CC0 source.
- Preserve all economy, population, construction, training, capture, combat, AI, pathfinding, fog, wall, gate, tower, victory, restart, and squad-HUD behavior.
- Do not add background music, voice lines, new units, new buildings, balance changes, ragdolls, physics destruction, weather, or a new presentation framework.
- Player/Enemy/Neutral remain blue/red/earth-yellow, encoded by both color and silhouette.
- UI and order confirmation sounds are 2D. Combat, construction, tower, and destruction sounds are attenuated 3D sounds with concurrency limits.
- Enemy world feedback must not reveal actors or locations hidden by fog of war.
- Missing required M4 references fail the asset audit; do not add runtime primitive or default-material fallbacks.
- The current branch contains accepted uncommitted M0-M3 work. Preserve it, do not create a clean worktree, and do not commit unless the user separately authorizes commits.
- Reduce test frequency: one RED compile in Task 2, one focused GREEN test gate after the presentation interfaces compile, and one final consolidated build/test/package gate. Asset-only tasks use audits instead of rebuilding the project.

---

## File Structure

**Create:**

- `docs/M4-ASSETS.md` — source, license, archive, selected file, role, and import destination ledger.
- `Art/M4ImportManifest.json` — machine-readable list consumed by import and audit scripts.
- `scripts/ImportM4Assets.py` — imports only manifest-listed models, textures, animations, and sounds.
- `scripts/ConfigureM4Presentation.py` — creates/configures materials, Data Assets, sound settings, and map presentation assets.
- `scripts/AuditM4Assets.py` — validates required roles, asset classes, references, skeleton compatibility, and source records.
- `Source/RTS/Variant_Strategy/StrategyPresentationActors.h` — cosmetic projectile actor declaration.
- `Source/RTS/Variant_Strategy/StrategyPresentationActors.cpp` — cosmetic projectile movement and lifetime.
- `Content/CityStateRTS/Data/DA_Presentation.uasset` — shared M4 presentation definition.
- `Content/CityStateRTS/Art/**` — selected imported and generated M4 content.
- `docs/verification/M4-assets.json` — asset-audit evidence.
- `docs/verification/M4-tests.json` — consolidated automation result.
- `docs/verification/M4-smoke.json` — packaged startup evidence.
- `docs/M4-CURRENT.md` — implementation result and manual acceptance checklist.

**Modify:**

- `Source/RTS/Variant_Strategy/StrategyTypes.h` — presentation enums and Data Asset properties.
- `Source/RTS/Variant_Strategy/StrategySystems.h/.cpp` — pure visual-state and fog-feedback rules.
- `Source/RTS/Variant_Strategy/StrategyGameState.h/.cpp` — load and expose the shared presentation definition.
- `Source/RTS/Variant_Strategy/StrategyUnit.h/.cpp` — skeletal rendering, animation, faction slot, combat feedback, delayed visual death, cavalry rider.
- `Source/RTS/Variant_Strategy/StrategyWorldActors.h/.cpp` — building construction, tower projectile, control-point flag/ring, capture and destruction feedback.
- `Source/RTS/Variant_Strategy/StrategyPlayerController.h/.cpp` — successful-order markers and command/invalid-action sounds.
- `Source/RTS/Variant_Strategy/StrategyMapTerrain.h/.cpp` — non-gameplay terrain presentation and HISM decoration while retaining existing blockers.
- `Source/RTS/Variant_Strategy/StrategyGameMode.cpp` — create the common presentation terrain actor on both maps.
- `Source/RTS/Variant_Strategy/UI/StrategyHUD.cpp` — unified HUD colors and state glyphs without changing marker interaction.
- `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp` — pure presentation rule tests.
- `Content/CityStateRTS/Data/DA_Unit_*.uasset` and `DA_Building_*.uasset` — wire type-specific M4 references.
- `Content/CityStateRTS/Maps/LVL_CityStateSkirmish.umap` and `LVL_RiverValleySkirmish.umap` — ground material and non-blocking decoration references.
- `docs/ROADMAP.md` — M4-01 evidence and status.

---

### Task 1: Acquire, select, and record the CC0 source assets

**Files:**

- Create outside repository: `D:\素材\游戏素材\CityStateRTS\M4\Quaternius\**`
- Create outside repository: `D:\素材\游戏素材\CityStateRTS\M4\PolyHaven\**`
- Create outside repository: `D:\素材\游戏素材\CityStateRTS\M4\Audio\**`
- Create: `docs/M4-ASSETS.md`
- Create: `Art/M4ImportManifest.json`

**Interfaces:**

- Produces a JSON document with `sourceRoot`, `sources`, and `assets` arrays.
- Each `assets` entry has `role`, `sourceFile`, `destination`, `assetName`, `license`, and `sourceUrl`; animation entries additionally have `skeletonRole` naming the mesh role whose skeleton they use.
- Required roles are `InfantryMesh`, `ArcherMesh`, `CavalryHorseMesh`, `CavalryRiderMesh`, sixteen unit animation roles, seven building meshes, `CapitalMesh`, `TownMesh`, `FlagMesh`, `TreeMeshA`, `TreeMeshB`, `RockMeshA`, `BridgeMesh`, `ArrowMesh`, four surface sets, and the eighteen sound roles listed below. The capture ring is a generated project material rather than a downloaded source asset.
- Consumed by `ImportM4Assets.py`, `ConfigureM4Presentation.py`, and `AuditM4Assets.py`.

- [x] **Step 1: Create the external source folders**

Create `Quaternius`, `PolyHaven`, and `Audio` under `D:\素材\游戏素材\CityStateRTS\M4`. Do not delete or overwrite unrelated files already present in `D:\素材\游戏素材`.

- [x] **Step 2: Download the approved Quaternius packs from their official pages**

Download and extract these packs into separate folders:

- `https://quaternius.com/packs/medievalvillagemegakit.html`
- `https://quaternius.com/packs/knightcharacter.html`
- `https://quaternius.com/packs/rpgcharacters.html`
- `https://quaternius.com/packs/ultimateanimatedanimals.html`
- `https://quaternius.com/packs/universalanimationlibrary.html`

Prefer FBX for Unreal import. Keep each pack's included license/readme beside the extracted files.

- [x] **Step 3: Download only four approved Poly Haven 2K surfaces**

From `https://polyhaven.com/textures`, select one stone, one wood-plank, one grass/mud ground, and one dirt-road material. Download 2K resolution with Base Color, Normal, and Roughness maps; include Displacement only if the selected ground material visibly needs it. Do not download 4K or 8K variants.

- [x] **Step 4: Download the approved Kenney sound packs**

Download and extract:

- `https://kenney.nl/assets/ui-audio`
- `https://kenney.nl/assets/interface-sounds`
- `https://kenney.nl/assets/rpg-audio`
- `https://kenney.nl/assets/impact-sounds`

Select one short file for each role: `Select`, `Move`, `AttackOrder`, `Invalid`, `ConstructionStart`, `ConstructionComplete`, `TrainingComplete`, `CaptureContested`, `CaptureComplete`, `MeleeHit`, `ArrowShot`, `ArrowHit`, `Hoof`, `TowerShot`, `BuildingHit`, `BuildingDestroyed`, `Victory`, and `Defeat`. `AttackOrder` is used for both attack-move and target attack, so eighteen unique sound files are sufficient.

- [x] **Step 5: Select exact model and animation files by semantic role**

Choose one coherent asset for each of these destinations:

```text
/Game/CityStateRTS/Art/Characters/Infantry/SK_Infantry
/Game/CityStateRTS/Art/Characters/Archer/SK_Archer
/Game/CityStateRTS/Art/Characters/Cavalry/SK_Horse
/Game/CityStateRTS/Art/Characters/Cavalry/SK_Rider
/Game/CityStateRTS/Art/Buildings/SM_Barracks
/Game/CityStateRTS/Art/Buildings/SM_ArcheryRange
/Game/CityStateRTS/Art/Buildings/SM_Stable
/Game/CityStateRTS/Art/Buildings/SM_House
/Game/CityStateRTS/Art/Buildings/SM_Tower
/Game/CityStateRTS/Art/Buildings/SM_Wall
/Game/CityStateRTS/Art/Buildings/SM_Gate
/Game/CityStateRTS/Art/Buildings/SM_Capital
/Game/CityStateRTS/Art/Buildings/SM_Town
```

For Infantry, Archer, Horse, and Rider, select `Idle`, `Move`, `Attack`, and `Death` animations compatible with that mesh's skeleton. The rider uses the humanoid animation set independently of the horse.

- [x] **Step 6: Write the human and machine-readable ledgers**

In `docs/M4-ASSETS.md`, create one row per selected file with Source Pack, Official URL, License, Download Date, Source File, Role, and UE Destination. In `Art/M4ImportManifest.json`, record the same selection using this schema:

```json
{
  "sourceRoot": "D:/素材/游戏素材/CityStateRTS/M4",
  "sources": [
    {"name": "Medieval Village MegaKit", "url": "https://quaternius.com/packs/medievalvillagemegakit.html", "license": "CC0-1.0"}
  ],
  "assets": [
    {
      "role": "BarracksMesh",
      "sourceFile": "Quaternius/MedievalVillageMegaKit/FBX/Buildings/Building_Barracks.fbx",
      "destination": "/Game/CityStateRTS/Art/Buildings",
      "assetName": "SM_Barracks",
      "license": "CC0-1.0",
      "sourceUrl": "https://quaternius.com/packs/medievalvillagemegakit.html"
    }
  ]
}
```

The shown source filename is an example of the completed form. Record the exact archive filename selected for the Barracks role; do not preserve an illustrative filename when it differs from the downloaded pack.

- [x] **Step 7: Run a manifest-only audit**

Run this PowerShell check once:

```powershell
$manifest = Get-Content -Raw 'D:\ue project\RTS\Art\M4ImportManifest.json' | ConvertFrom-Json
$missing = $manifest.assets | Where-Object { -not (Test-Path (Join-Path $manifest.sourceRoot $_.sourceFile)) }
$invalid = $manifest.assets | Where-Object { $_.license -ne 'CC0-1.0' -or $_.sourceUrl -notmatch '^https://' }
if ($missing -or $invalid) { throw 'M4 source manifest contains missing or non-CC0 entries.' }
```

Expected: no output and exit code `0`.

---

### Task 2: Add presentation data contracts and pure visibility/state rules

**Files:**

- Modify: `Source/RTS/Variant_Strategy/StrategyTypes.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.h`
- Modify: `Source/RTS/Variant_Strategy/StrategySystems.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameState.cpp`
- Modify: `Source/RTS/Variant_Strategy/Tests/StrategySystemsTests.cpp`

**Interfaces:**

- Produces `EStrategyUnitVisualState { Idle, Move, Attack, Dead }`.
- Produces `EStrategyUnitVisualState FStrategyPresentationRules::ResolveUnitVisualState(bool bAlive, bool bAttacking, float SpeedSquared)`.
- Produces `bool FStrategyPresentationRules::CanPlayWorldFeedback(EStrategyFaction SourceFaction, bool bVisibleToPlayer)`.
- Extends `UStrategyUnitDataAsset` with `SkeletalMesh`, `FactionMaterialSlot`, four primary animations, optional rider mesh, `RiderFactionMaterialSlot`, four rider animations, `ProjectileMesh`, `AttackSound`, `HitSound`, `AttackVisualDuration`, and `DeathVisualDuration`.
- Extends `UStrategyBuildingDataAsset` with `FactionMaterialSlot`, `ProjectileMesh`, `AttackSound`, `HitSound`, and `DestroyedSound`.
- Produces `UStrategyPresentationDataAsset` with shared faction materials, capital/town/flag/environment meshes, command Niagara systems, state Niagara systems, shared sounds, attenuation, and concurrency references.
- Produces `const UStrategyPresentationDataAsset* AStrategyGameState::GetPresentationDefinition()`.

- [x] **Step 1: Write failing visual-state tests**

Add `FStrategyPresentationRulesTest` to `StrategySystemsTests.cpp` with these assertions:

```cpp
TestEqual(TEXT("死亡优先"), FStrategyPresentationRules::ResolveUnitVisualState(false, true, 100.0f), EStrategyUnitVisualState::Dead);
TestEqual(TEXT("攻击优先于移动"), FStrategyPresentationRules::ResolveUnitVisualState(true, true, 100.0f), EStrategyUnitVisualState::Attack);
TestEqual(TEXT("有速度时移动"), FStrategyPresentationRules::ResolveUnitVisualState(true, false, 1.0f), EStrategyUnitVisualState::Move);
TestEqual(TEXT("静止时待机"), FStrategyPresentationRules::ResolveUnitVisualState(true, false, 0.0f), EStrategyUnitVisualState::Idle);
```

- [x] **Step 2: Write failing fog-feedback tests**

Add assertions that Player feedback is allowed regardless of `bVisibleToPlayer`, Enemy feedback is allowed only when visible, and Neutral feedback follows `bVisibleToPlayer`.

- [x] **Step 3: Run the single RED compile**

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\Build.bat' RTSEditor Win64 Development 'D:\ue project\RTS\RTS.uproject' -WaitMutex -NoHotReload
```

Expected: compile failure because `EStrategyUnitVisualState` and `FStrategyPresentationRules` do not exist.

- [x] **Step 4: Add the minimal enums, properties, and forward declarations**

Use `TObjectPtr` for asset references and `TSubclassOf` only where Unreal requires a class. Use `UAnimationAsset` rather than an Animation Blueprint so each unit can play the four explicit single-node states without creating a second state-machine layer.

Add these exact unit presentation properties:

```cpp
TObjectPtr<USkeletalMesh> SkeletalMesh;
FName FactionMaterialSlot = TEXT("Faction");
TObjectPtr<UAnimationAsset> IdleAnimation;
TObjectPtr<UAnimationAsset> MoveAnimation;
TObjectPtr<UAnimationAsset> AttackAnimation;
TObjectPtr<UAnimationAsset> DeathAnimation;
TObjectPtr<USkeletalMesh> RiderSkeletalMesh;
FName RiderFactionMaterialSlot = TEXT("Faction");
TObjectPtr<UAnimationAsset> RiderIdleAnimation;
TObjectPtr<UAnimationAsset> RiderMoveAnimation;
TObjectPtr<UAnimationAsset> RiderAttackAnimation;
TObjectPtr<UAnimationAsset> RiderDeathAnimation;
TObjectPtr<UStaticMesh> ProjectileMesh;
TObjectPtr<USoundBase> AttackSound;
TObjectPtr<USoundBase> HitSound;
float AttackVisualDuration = 0.35f;
float DeathVisualDuration = 0.8f;
```

Add these exact building presentation properties:

```cpp
FName FactionMaterialSlot = TEXT("Faction");
TObjectPtr<UStaticMesh> ProjectileMesh;
TObjectPtr<USoundBase> AttackSound;
TObjectPtr<USoundBase> HitSound;
TObjectPtr<USoundBase> DestroyedSound;
```

Add these exact shared presentation properties to `UStrategyPresentationDataAsset`:

```cpp
TObjectPtr<UMaterialInterface> PlayerFactionMaterial;
TObjectPtr<UMaterialInterface> EnemyFactionMaterial;
TObjectPtr<UMaterialInterface> NeutralFactionMaterial;
TObjectPtr<UMaterialInterface> ConstructionMaterial;
TObjectPtr<UMaterialInterface> HitFlashMaterial;
TObjectPtr<UMaterialInterface> CaptureRingMaterial;
TObjectPtr<UStaticMesh> CapitalMesh;
TObjectPtr<UStaticMesh> TownMesh;
TObjectPtr<UStaticMesh> FlagMesh;
TObjectPtr<UStaticMesh> TreeMeshA;
TObjectPtr<UStaticMesh> TreeMeshB;
TObjectPtr<UStaticMesh> RockMeshA;
TObjectPtr<UStaticMesh> BridgeMesh;
TObjectPtr<UNiagaraSystem> MoveCommandEffect;
TObjectPtr<UNiagaraSystem> AttackMoveCommandEffect;
TObjectPtr<UNiagaraSystem> AttackTargetEffect;
TObjectPtr<UNiagaraSystem> HitEffect;
TObjectPtr<UNiagaraSystem> ConstructionEffect;
TObjectPtr<UNiagaraSystem> ConstructionCompleteEffect;
TObjectPtr<UNiagaraSystem> DestructionEffect;
TObjectPtr<UNiagaraSystem> CaptureEffect;
TObjectPtr<UNiagaraSystem> ProjectileTrailEffect;
TObjectPtr<USoundBase> SelectSound;
TObjectPtr<USoundBase> MoveSound;
TObjectPtr<USoundBase> AttackOrderSound;
TObjectPtr<USoundBase> InvalidSound;
TObjectPtr<USoundBase> ConstructionStartSound;
TObjectPtr<USoundBase> ConstructionCompleteSound;
TObjectPtr<USoundBase> TrainingCompleteSound;
TObjectPtr<USoundBase> CaptureContestedSound;
TObjectPtr<USoundBase> CaptureCompleteSound;
TObjectPtr<USoundBase> MeleeHitSound;
TObjectPtr<USoundBase> ArrowShotSound;
TObjectPtr<USoundBase> ArrowHitSound;
TObjectPtr<USoundBase> HoofSound;
TObjectPtr<USoundBase> TowerShotSound;
TObjectPtr<USoundBase> BuildingHitSound;
TObjectPtr<USoundBase> BuildingDestroyedSound;
TObjectPtr<USoundBase> VictorySound;
TObjectPtr<USoundBase> DefeatSound;
TObjectPtr<USoundAttenuation> WorldAttenuation;
TObjectPtr<USoundConcurrency> CombatConcurrency;
```

Set exact default durations:

```cpp
float AttackVisualDuration = 0.35f;
float DeathVisualDuration = 0.8f;
```

- [x] **Step 5: Implement the pure rules**

```cpp
EStrategyUnitVisualState FStrategyPresentationRules::ResolveUnitVisualState(bool bAlive, bool bAttacking, float SpeedSquared)
{
    if (!bAlive) return EStrategyUnitVisualState::Dead;
    if (bAttacking) return EStrategyUnitVisualState::Attack;
    return SpeedSquared > KINDA_SMALL_NUMBER ? EStrategyUnitVisualState::Move : EStrategyUnitVisualState::Idle;
}

bool FStrategyPresentationRules::CanPlayWorldFeedback(EStrategyFaction SourceFaction, bool bVisibleToPlayer)
{
    return SourceFaction == EStrategyFaction::Player || bVisibleToPlayer;
}
```

Register the test under the exact name `RTS.Strategy.Systems.PresentationRules`.

- [x] **Step 6: Load the shared presentation Data Asset**

Add the fixed path:

```cpp
constexpr const TCHAR* PresentationPath = TEXT("/Game/CityStateRTS/Data/DA_Presentation.DA_Presentation");
```

Load it in `EnsureDefinitionsLoaded()` after the existing unit/building definitions and expose it through `GetPresentationDefinition()`. Use `checkf` with a Chinese message, matching the existing required Data Asset behavior.

- [x] **Step 7: Run the focused GREEN gate**

Build `RTSEditor Win64 Development`, then run `RTS.Strategy.Systems.PresentationRules`. Expected: build succeeds and the new test reports success. Do not package at this gate.

---

### Task 3: Import and configure the selected M4 content reproducibly

**Files:**

- Create: `scripts/ImportM4Assets.py`
- Create: `scripts/ConfigureM4Presentation.py`
- Create: `scripts/AuditM4Assets.py`
- Create/Modify: `Content/CityStateRTS/Art/**`
- Create: `Content/CityStateRTS/Data/DA_Presentation.uasset`
- Modify: `Content/CityStateRTS/Data/DA_Unit_*.uasset`
- Modify: `Content/CityStateRTS/Data/DA_Building_*.uasset`
- Modify: `Content/CityStateRTS/Maps/LVL_CityStateSkirmish.umap`
- Modify: `Content/CityStateRTS/Maps/LVL_RiverValleySkirmish.umap`

**Interfaces:**

- Consumes every entry in `Art/M4ImportManifest.json`.
- Produces stable `/Game/CityStateRTS/Art/...` paths and `DA_Presentation` references expected by Task 2.
- Produces `docs/verification/M4-assets.json` with `missingRoles`, `missingAssets`, `classMismatches`, `skeletonMismatches`, `missingReferences`, `unrecordedSources`, and `success`.

- [x] **Step 1: Implement manifest-driven import**

In `ImportM4Assets.py`, read the UTF-8 JSON manifest, create one `unreal.AssetImportTask` per entry, set `automated = True`, `replace_existing = True`, `save = True`, and import to the declared destination. Configure FBX skeletal imports only for character/animation roles; import building/environment roles as static meshes; import WAV files as sound waves; import PNG/JPG/EXR files as textures.

- [x] **Step 2: Enforce stable asset names**

After each import, rename the primary produced asset to the manifest's `assetName`. Animation assets must use these exact names per type:

```text
A_Infantry_Idle, A_Infantry_Move, A_Infantry_Attack, A_Infantry_Death
A_Archer_Idle, A_Archer_Move, A_Archer_Attack, A_Archer_Death
A_Horse_Idle, A_Horse_Move, A_Horse_Attack, A_Horse_Death
A_Rider_Idle, A_Rider_Move, A_Rider_Attack, A_Rider_Death
```

- [x] **Step 3: Create shared materials and faction instances**

In `ConfigureM4Presentation.py`, create one opaque surface master material with Base Color, Normal, Roughness, and `FactionColor` parameters, plus one translucent construction material, one translucent hit-flash overlay, and one deferred-decal capture-ring material with `Progress`, `FactionColor`, and `Contested` parameters. Create `MI_Faction_Player`, `MI_Faction_Enemy`, and `MI_Faction_Neutral` with the existing Strategy colors `(0.10,0.34,0.58)`, `(0.61,0.18,0.14)`, and `(0.68,0.54,0.32)`. Create the four selected Poly Haven material instances for stone, wood, ground, and road.

- [x] **Step 4: Create audio attenuation and concurrency assets**

Create `ATT_WorldFeedback` with an inner radius of `600 cm`, a falloff distance of `3000 cm`, and logarithmic attenuation. Create `SC_CombatFeedback` with a maximum concurrent count of `12`, resolution rule `StopFarthestThenPreventNew`, and volume scaling enabled. Assign them to combat, construction, tower, and destruction sounds. Leave selection/order sounds as 2D assets.

- [x] **Step 5: Configure type-specific Data Assets**

Set the exact imported skeletal mesh and four animation references on each `DA_Unit_*`. Configure Horse and Rider references on `DA_Unit_Cavalry`. Set imported static meshes on all seven existing `DA_Building_*`; set tower and archer projectile mesh to `SM_Arrow`; set `FactionMaterialSlot` names according to the imported mesh slots. Do not change gameplay properties such as costs, health, damage, range, construction time, footprint, population, or trainable units.

- [x] **Step 6: Configure `DA_Presentation`**

Assign faction, construction, hit-flash, and capture-ring materials; capital/town/flag meshes; tree/rock/bridge/environment meshes; command, construction, completion, hit, destruction, capture, and projectile-trail effects; all shared sound roles; `ATT_WorldFeedback`; and `SC_CombatFeedback`.

- [x] **Step 7: Apply map presentation without changing navigation**

Assign the new ground material to both map ground meshes. Do not move navigation volumes, control-point coordinates, capitals, blockers, bridges, fords, or no-build zones. Save both maps using Unreal Editor APIs, never by binary copying `.umap` files.

- [x] **Step 8: Implement the asset audit**

`AuditM4Assets.py` must load all required asset paths and verify:

- every manifest role exists and resolves to the expected Unreal class;
- every unit has mesh plus four animations;
- horse and rider each have four compatible animations;
- all seven building Data Assets have non-null meshes;
- `DA_Presentation` has all required faction, command, state, control-point, environment, sound, attenuation, and concurrency references;
- every imported path has a matching CC0 row in the manifest;
- no formal unit/building definition points to `/Engine/BasicShapes`.

Write the result to `docs/verification/M4-assets.json` and raise `RuntimeError` when `success` is false.

- [x] **Step 9: Run import, configuration, and audit once**

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -run=pythonscript -script='D:/ue project/RTS/scripts/ImportM4Assets.py' -unattended -nop4 -nosplash
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -run=pythonscript -script='D:/ue project/RTS/scripts/ConfigureM4Presentation.py' -unattended -nop4 -nosplash
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -run=pythonscript -script='D:/ue project/RTS/scripts/AuditM4Assets.py' -unattended -nop4 -nosplash
```

Expected: all three commands exit `0`; audit JSON contains `"success": true`.

---

### Task 4: Replace unit rendering and connect animation/combat feedback

**Files:**

- Modify: `Source/RTS/Variant_Strategy/StrategyUnit.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyUnit.cpp`

**Interfaces:**

- Consumes `UStrategyUnitDataAsset` presentation fields and `FStrategyPresentationRules`.
- Produces private `UpdatePresentation(float DeltaSeconds)`, `PlayVisualState(EStrategyUnitVisualState NewState)`, `PlayAttackFeedback()`, `PlayHitFeedback()`, and `BeginDeathPresentation()` methods.
- Preserves the public `Initialize`, `IssueOrder`, `ReceiveStrategyDamage`, selection, movement, and damageable interfaces.

- [x] **Step 1: Activate the inherited skeletal mesh**

In the constructor, create `RiderMesh` as a `USkeletalMeshComponent` attached to `GetMesh()`. Keep `BodyMesh` so existing serialized actors remain loadable, but hide it for every configured M4 unit. Configure `GetMesh()` for no collision and visible rendering.

- [x] **Step 2: Apply unit presentation from the Data Asset**

Add `TObjectPtr<const UStrategyUnitDataAsset> Definition` to `AStrategyUnit`. In `Initialize`, store `InDefinition`, assign the skeletal mesh, relative transform, single-node animation mode, and faction material only to the configured `FactionMaterialSlot`. For cavalry, assign and attach the Rider mesh at the selected saddle socket or fixed relative transform, then apply its faction slot. Do not tint natural material slots.

- [x] **Step 3: Drive the four visual states**

Call `UpdatePresentation` from `Tick` after combat. Resolve state from `Health > 0`, `AttackVisualLockRemaining > 0`, and `GetVelocity().SizeSquared2D()`. Play Idle and Move looping; play Attack and Death once. Do not restart an animation when the resolved state equals the current state.

- [x] **Step 4: Trigger attack and projectile feedback from the existing damage moment**

Immediately before the existing `ReceiveStrategyDamage` call in `UpdateCombat`, set `AttackVisualLockRemaining = Definition->AttackVisualDuration`, play Attack on the main mesh and rider, and trigger one attack sound. Archer units spawn the cosmetic arrow from their weapon/socket position toward the target through `AStrategyProjectileVisual`; damage timing and amount remain unchanged.

- [x] **Step 5: Add hit feedback without changing damage rules**

After applying the existing counter multiplier in `ReceiveStrategyDamage`, apply `HitFlashMaterial` as an overlay for `0.08` seconds, spawn the hit effect, and play one concurrency-limited hit sound only when `CanPlayWorldFeedback` permits it. Do not alter the multiplier, source-faction check, or health calculation.

- [x] **Step 6: Delay actor destruction only for the death presentation**

When health reaches zero, notify the squad immediately, disable movement/collision/targeting immediately, play Death once, spawn one small dust effect, and call `SetLifeSpan(DeathVisualDuration)` instead of immediate `Destroy()`. `IsStrategyAlive()` remains false from the same frame, so target selection, population release, victory flow, and combat rules see the unit as dead immediately.

- [x] **Step 7: Review the unit diff without rebuilding**

Inspect that gameplay assignments to `Health`, `Damage`, `AttackInterval`, `AttackRange`, `MoveSpeed`, and the existing order path are unchanged. Confirm `BodyMesh` no longer loads `/Engine/BasicShapes` for configured M4 units and no new fallback branch was introduced.

---

### Task 5: Add cosmetic projectiles, building states, and control-point feedback

**Files:**

- Create: `Source/RTS/Variant_Strategy/StrategyPresentationActors.h`
- Create: `Source/RTS/Variant_Strategy/StrategyPresentationActors.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyWorldActors.cpp`

**Interfaces:**

- Produces `void AStrategyProjectileVisual::Initialize(UStaticMesh* Mesh, UNiagaraSystem* TrailEffect, const FVector& Start, const FVector& End, float Duration)`.
- Produces `float AStrategyBuilding::GetConstructionProgress() const` as `1.0` when complete, otherwise clamped `ConstructionElapsed / ConstructionTime`.
- Consumes `AStrategyGameState::GetPresentationDefinition()` and building/unit Data Asset presentation fields.
- Preserves all building, gate collision, tower targeting, capture, population, and cleanup interfaces.

- [x] **Step 1: Implement the cosmetic projectile actor**

Create a ticking actor with one no-collision `UStaticMeshComponent` and an optional attached Niagara trail. `Initialize` stores Start, End, and `max(Duration, 0.05f)`. Tick linearly interpolates location, points the mesh along travel direction, and destroys the actor at alpha `1.0`. It never performs collision, targeting, or damage.

- [x] **Step 2: Replace formal building appearance without changing footprints**

Use `Definition->VisualMesh` directly and remove the visible `/Engine/BasicShapes` fallback. Keep the existing `UBoxComponent` extents, wall line planner, gate posts, friendly collision channel, navigation relevance, and building transforms. If the selected gate is a complete mesh, hide legacy gate posts and use the main mesh for the visible gate while retaining the existing box collision.

- [x] **Step 3: Add construction presentation**

During construction, apply the shared translucent construction material and expose the existing `ConstructionElapsed / ConstructionTime` through `GetConstructionProgress()`. Keep current height growth only if it improves readability with the chosen mesh; otherwise hold final scale and animate material opacity. Play `ConstructionStart` and a small dust effect once from `Initialize`. On `CompleteConstruction`, restore the formal materials and play one completion dust effect plus `ConstructionComplete`.

- [x] **Step 4: Add tower firing feedback**

In the existing `BestTarget` success block, spawn a cosmetic arrow from the tower platform to the target and play `TowerShot`, then execute the unchanged `25` damage and `1.2` second cooldown assignments. The arrow actor remains cosmetic and does not repeat damage.

- [x] **Step 5: Add building hit and destruction feedback**

In `ReceiveStrategyDamage`, apply `HitFlashMaterial` as a brief overlay and trigger a short hit effect/sound after the existing friendly-fire/alive guard. Clear the overlay after `0.08` seconds. When health reaches zero, play dust/debris and destruction sound before `ApplyCleanup` and `Destroy`. Gate, population bonus, queue reservation release, and unregister logic remain unchanged.

- [x] **Step 6: Play training completion feedback**

In the existing `TrainingQueue.Update` success block, play `TrainingCompleteSound` once at the production building after the squad is spawned. Do not change queue order, spawn position, population commit, or training duration.

- [x] **Step 7: Replace capital/town visuals and add flag/ring components**

Add a no-collision `FlagMesh` and a `UDecalComponent` named `CaptureRing` to `AStrategyControlPoint`. Use `CapitalMesh` or `TownMesh` from `DA_Presentation`; apply only the faction material slot and flag material. Create a dynamic instance from `CaptureRingMaterial`; hide it for capitals and update its `Progress` parameter from `CaptureState.ProgressSeconds / 10.0f` for towns.

- [x] **Step 8: Connect contested and completed capture feedback**

Track the previous challenger/contested state locally for presentation only. Flash the ring and play `CaptureContested` once when both factions first contest. When owner changes in the existing block, update the flag/material, play `CaptureComplete`, and spawn the capture effect. Do not change `FStrategyCaptureState::Update` or its timing.

- [x] **Step 9: Gate enemy world feedback through fog**

Before enemy/neutral world sound or FX creation, query `IsVisibleToFaction(EStrategyFaction::Player, Location)` and pass it to `CanPlayWorldFeedback`. Player-owned feedback remains audible; hidden enemy construction, combat, capture, and destruction feedback is not spawned for the player.

- [x] **Step 10: Review the world-actor diff without rebuilding**

Confirm tower range `1500`, damage `25`, interval `1.2`, capture duration `10`, capital health `3000`, construction times, gate collision channels, and cleanup paths are unchanged.

---

### Task 6: Connect command markers, HUD styling, and environment presentation

**Files:**

- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyPlayerController.cpp`
- Modify: `Source/RTS/Variant_Strategy/UI/StrategyHUD.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyMapTerrain.h`
- Modify: `Source/RTS/Variant_Strategy/StrategyMapTerrain.cpp`
- Modify: `Source/RTS/Variant_Strategy/StrategyGameMode.cpp`

**Interfaces:**

- Produces private `ShowOrderFeedback(const FStrategyOrder& Order)`, `PlayInvalidActionFeedback()`, and `UpdateMatchResultFeedback()` on the controller.
- Produces `void AStrategyMapTerrain::InitializePresentation(const FStrategySkirmishMapDefinition& Definition)`.
- Preserves `DoIssueOrder`, squad-marker drag semantics, right-click behavior, build placement, wall drag, camera input, fog grid, map routes, and blockers.

- [x] **Step 1: Trigger one marker per successful player order**

At the end of `DoIssueOrder`, after at least one controlled squad receives the order, call `ShowOrderFeedback` once. Select Move/AttackMove/AttackTarget Niagara assets and 2D sounds from `DA_Presentation`; use green/orange/red assets respectively. Spawn at `Order.Destination` or the target actor location and configure the effect lifetime to approximately `0.8` seconds.

- [x] **Step 2: Preserve both order input paths**

Do not add marker logic to individual mouse handlers. Keeping it only in `DoIssueOrder` ensures ordinary right-click and projected squad-marker drag use the same feedback path and still obey the established rule: selected squads move together; dragging an unselected squad first selects and moves only that squad.

- [x] **Step 3: Add invalid action audio only where the existing action returns failure**

Play `Invalid` once for rejected final building placement, rejected gate upgrade, or a command with no controllable squad. Do not add input filtering, cooldown validation, or new gameplay guards.

- [x] **Step 4: Add selection and match-result feedback**

Play `SelectSound` once after a player selection operation leaves at least one squad or one building selected. Track `bMatchResultFeedbackPlayed` in the controller; when the GameState winner first becomes non-neutral, play `VictorySound` for Player or `DefeatSound` for Enemy exactly once. Reset the flag naturally with the controller when `R` reloads the level.

- [x] **Step 5: Restyle HUD drawing without altering hit regions**

Replace debug-like colors with the shared player/enemy/neutral palette and distinct move/attack glyph shapes. Draw a compact projected construction bar above incomplete buildings using `GetConstructionProgress()`. Keep all squad marker projection, marker bounds, drag hit testing, selection box, health ratio, resource text, queue text, and click/drag thresholds numerically unchanged; the construction bar is display-only and has no hit region.

- [x] **Step 6: Separate river collision from visible environment geometry**

Retain the current invisible cube collision blocks at the same transforms for navigation and unit blocking. Replace their visible shapes with non-colliding water, bridge, ford, tree, and rock meshes from `DA_Presentation`. No visual mesh may become the authoritative blocker.

- [x] **Step 7: Add low-count HISM decoration**

Use `UHierarchicalInstancedStaticMeshComponent` for `TreeMeshA`, `TreeMeshB`, and `RockMeshA`. Place a small deterministic, symmetric set outside capital clear zones, town capture radii, primary paths, bridge/ford openings, and building territory centers. Disable collision and navigation relevance for these decorative instances.

- [x] **Step 8: Initialize presentation terrain on both maps**

Spawn one `AStrategyMapTerrain` for both maps and call `InitializePresentation(MapDefinition)`. Inside it, add common decoration for both maps and call the existing river layout only when `bSpawnRiverValleyTerrain` is true.

- [x] **Step 9: Review command and map diffs without rebuilding**

Confirm W/S/A/D camera direction, right-click movement, F attack-move, X stop, drag-marker input, selection behavior, river crossings, no-build zones, and World Partition navigation settings are untouched.

---

### Task 7: Run the consolidated verification gate and prepare M4-01 acceptance

**Files:**

- Create: `docs/verification/M4-tests.json`
- Create: `docs/verification/M4-smoke.json`
- Create: `docs/M4-CURRENT.md`
- Modify: `docs/ROADMAP.md`

**Interfaces:**

- Consumes all Tasks 1-6 deliverables.
- Produces a single Windows M4-01 candidate and the manual acceptance checklist.

- [x] **Step 1: Run the final asset audit**

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -run=pythonscript -script='D:/ue project/RTS/scripts/AuditM4Assets.py' -unattended -nop4 -nosplash
```

Expected: exit code `0`, `M4-assets.json` reports `success: true`, and no formal unit/building mesh references `/Engine/BasicShapes`.

- [x] **Step 2: Run one consolidated editor build**

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\Build.bat' RTSEditor Win64 Development 'D:\ue project\RTS\RTS.uproject' -WaitMutex -NoHotReload
```

Expected: `Result: Succeeded`.

- [x] **Step 3: Run the affected Strategy automation tests once**

Run all `RTS.Strategy` tests once in one unattended editor session:

```powershell
& 'D:\ue5\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\ue project\RTS\RTS.uproject' -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests RTS.Strategy' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/ue project/RTS/Saved/Verification/M4/Automation' '-abslog=D:/ue project/RTS/Saved/Verification/M4/Automation.log' -nosplash
Copy-Item -LiteralPath 'D:\ue project\RTS\Saved\Verification\M4\Automation\index.json' -Destination 'D:\ue project\RTS\docs\verification\M4-tests.json' -Force
```

Expected: editor exit code `0`; every test succeeds; presentation tests confirm state priority and fog-feedback rules; existing gameplay tests remain green.

- [ ] **Step 4: Perform one consolidated PIE review**

Check both maps in one editor session:

- blue/red/neutral identity is readable without debug display;
- Infantry, Archer, and Cavalry silhouettes and four animation states are distinct;
- every building is identifiable, wall segments remain continuous, and the gate remains friendly-only;
- Move, AttackMove, and AttackTarget markers differ by shape and color;
- construction, training, capture, tower attack, damage, death, building destruction, victory, and defeat produce the intended feedback;
- squad HUD drag and ordinary right-click still issue orders;
- hidden enemy actors, particles, and spatial sounds do not leak through fog;
- two 60-pop forces remain selectable and readable without sustained audio overload.

- [x] **Step 5: Package the Windows candidate once**

Package Development Win64 once:

```powershell
& 'D:\ue5\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun '-project=D:/ue project/RTS/RTS.uproject' -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive '-archivedirectory=D:/ue project/RTS/Builds/Windows_M4_VisualAudio' -utf8output
```

Expected: packaging reports `BUILD SUCCESSFUL` and `D:\ue project\RTS\Builds\Windows_M4_VisualAudio\RTS.exe` exists.

- [x] **Step 6: Run one packaged smoke session**

Launch the packaged executable for at least `20` seconds, issue a normal right-click order and one squad-marker drag, then exit normally. Scan the current log for `Fatal error`, `Assertion failed`, `Unhandled Exception`, `Access violation`, missing package, and asset load failure. Write duration, executable path, exit code, scanned patterns, and counts to `docs/verification/M4-smoke.json`.

- [x] **Step 7: Document the candidate**

Create `docs/M4-CURRENT.md` with selected source packs, implemented visual/audio changes, audit/build/test/package evidence, candidate path, known presentation limitations, and a concise two-map manual checklist. Do not claim a complete match was verified unless it was actually played.

- [x] **Step 8: Update roadmap status**

Set M4-01 to `待验收` after all automated and smoke evidence passes. Set it to `完成` only after the user confirms the packaged candidate's visual/audio checklist and full-match behavior.

---

## Stage Review Order

Use one fresh implementation worker per task and review in this order:

1. Task 1 license/selection review.
2. Task 2 data-contract and rule review, followed by the focused GREEN gate.
3. Task 3 import/audit review.
4. Task 4 unit presentation review.
5. Task 5 building/control-point review.
6. Task 6 input/HUD/environment regression review.
7. Task 7 consolidated evidence review.

For Tasks 4-6, use static diff and editor asset inspection at the individual review gates; defer repeated builds and packages to Task 7 unless a compile error prevents progress.
