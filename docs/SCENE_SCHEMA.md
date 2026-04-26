# BlueFrog Scene JSON Schema (v2)

This document describes the on-disk format for scenes and prefabs used by `SceneLoader`, `PrefabLoader`, and `SceneSerializer`. The schema is deliberately small; extend it only when a real consumer exists.

- Scene files live in `BlueFrog/Assets/Scenes/*.json`.
- Prefab files live in `BlueFrog/Assets/Prefabs/*.prefab.json`.
- Relative paths are resolved against the **current working directory** at launch (typically `$(ProjectDir)` = `BlueFrog/BlueFrog/` under Visual Studio). Move the CWD and scene/prefab references break the same way.
- All three modules use [`nlohmann/json`](https://github.com/nlohmann/json); trailing commas, comments, and other JSON5 features are not supported.

## Save (F12)

`SceneSerializer::Save` writes the live `Scene` + camera target + `ObjectiveState` back to JSON in the same v2 shape `SceneLoader` consumes. App binds it to **F12**, targeting the current scene path (`currentScenePath` — what booted, plus any trigger-driven transitions). The save / reload pair (F12 → F5) closes the editor round-trip: tweak via the F2 inspector, F12 to persist, F5 to verify.

Lossiness in v1:

- **Prefab references are not preserved.** Objects originally loaded via `"prefab": "..."` get their merged form serialized into the scene file; the prefab reference is dropped. Loading the saved file works, but subsequent prefab edits no longer flow through to that object.
- **Runtime mutation persists.** Anything currently in the live scene is what gets written: a partially-killed enemy is saved with its current HP, a fired trigger is saved with `fired: true` would have been lossy if we wrote it (we don't — see below). Save before gameplay-induced mutations if you want pristine data.
- **Runtime-scratch fields are dropped on save:** `CombatComponent::attackCooldownRemaining`, `TriggerComponent::fired`, `ObjectiveLeaf::progress`. The reload always starts fresh.
- **Float formatting expands.** nlohmann's `dump(2)` writes floats with full IEEE precision (`1.350000023841858` instead of `1.35`). Hand-authored files stay terse; saved files are noisier but functionally identical on reload.
- **`scene.name`** is not round-tripped (the loader treats it as informational and never stores it).

## Top-level scene

```json
{
  "schemaVersion": 2,
  "objective": {
    "text": "Defeat the scout",
    "completionText": "Arena cleared",
    "conditions": [
      { "type": "enemy_killed", "name": "EnemyScout" }
    ]
  },
  "scene": {
    "name": "ArenaTrial",
    "camera": { "target": [-4.0, 1.25, 0.0] },
    "objects": [ /* ... */ ]
  }
}
```

| Key | Required | Notes |
|---|---|---|
| `schemaVersion` | yes | Integer. `1` and `2` are both accepted. `1` is the subset that existed before Phase 5 introduced `prefab`/`trigger`. New scenes should write `2`. |
| `objective` | no | Scene-level objective shown in the title bar. Omit for scenes without a goal — the title segment renders empty. See *Objective block* below. |
| `scene.name` | no | Informational. Not used at runtime. |
| `scene.camera.target` | no | `[x, y, z]` world-space point the camera follows on spawn. Defaults to the engine's existing camera state if omitted. |
| `scene.objects` | no | Array. Missing array = empty scene. |

### Objective block

Drives the dynamic objective text segment of the title bar. `ObjectiveSystem` consumes `EnemyKilled` events from the `EventBus` every tick and flips matching conditions to met; once every condition is met, the HUD switches from `text` to `completionText`.

```json
"objective": {
  "text": "Defeat both scouts",
  "completionText": "Yard cleared",
  "conditions": [
    { "type": "enemy_killed", "name": "EnemyScout" },
    { "type": "enemy_killed", "name": "EnemyScoutElite" }
  ]
}
```

| Field | Values | Default |
|---|---|---|
| `text` | In-progress title string (ASCII). | `""` |
| `completionText` | Shown once all conditions are met. | `""` |
| `conditions` | Array of condition slots (**AND-combined** at the top level). Empty or missing = trivially complete (title always shows `completionText`). | `[]` |

Each entry in `conditions` is one of:

**1. Single leaf (v1 shape, unchanged):**

```json
{ "type": "enemy_killed", "name": "EnemyScout" }
{ "type": "enemy_killed", "name": "EnemyScout", "count": 3 }
```

| Field | Values | Notes |
|---|---|---|
| `type` | `"enemy_killed"` | Allow-list enforced by `SceneLoader::Validate`; typoed types reject at startup with a path-prefixed error. |
| `name` | Scene-object name to match. | For `enemy_killed`, the defeated object's `name` must equal this string. |
| `count` | Integer ≥ 1. | How many matching events the slot needs before it counts as met. Defaults to `1` (v1 single-kill). Negative or zero rejects at validate. |

**2. OR group (`any`):**

```json
{
  "type": "any",
  "anyOf": [
    { "type": "enemy_killed", "name": "EnemyScout", "count": 2 },
    { "type": "enemy_killed", "name": "EnemyElite" }
  ]
}
```

| Field | Values | Notes |
|---|---|---|
| `type` | `"any"` | Marks the slot as an OR group. The slot has no `name`/`count` of its own. |
| `anyOf` | Non-empty array of leaves (same shape as case 1). | Slot is met as soon as **any** leaf is met. Each leaf accumulates progress independently — useful for "Kill 3 scouts OR 1 elite". |

The full evaluation is `AND_i (OR_j leaves[i][j])`. To express "Kill A AND (B OR C)": one leaf slot for A, one `any` slot listing B and C. To express plain AND of multiple kills: one leaf slot per kill, no `any`.

Per-leaf `count` is independent: each leaf has its own `progress` counter that climbs from `0` to `required`, capped (so re-publishing the same kill event does not over-count). Scene reload resets every counter via `ObjectiveSystem::Reset`.

## Scene object

Each entry in `scene.objects` maps to one `SceneObject`. Component keys are all optional; absent keys produce an object without that component.

```json
{
  "name": "Player",
  "prefab": "Assets/Prefabs/Player.prefab.json",
  "transform": { "position": [-4.0, 1.25, 0.0], "rotation": [0.0, 1.5708, 0.0], "scale": [0.7, 1.25, 0.7] },
  "render":    { "mesh": "cube", "material": { "tint": [1, 1, 1] } },
  "collision": { "halfExtents": [0.45, 0.45], "blocking": true },
  "combat":    { "faction": "player", "maxHp": 5, "currentHp": 5 },
  "trigger":   { "halfExtents": [1.5, 1.5], "tag": "center_zone", "fireOnce": true },
  "behavior":  { "type": "archer" }
}
```

| Key | Purpose |
|---|---|
| `name` | Required for objects the gameplay code looks up by name (`GameplaySceneIds::Player`, `GameplaySceneIds::EnemyScout`). Others may use any label or be empty. |
| `prefab` | Optional. Path to a `*.prefab.json`. See *Prefab merge* below. |
| `transform` | See *Transform*. |
| `render` | See *Render component*. |
| `collision` | See *Collision component*. |
| `combat` | See *Combat component*. |
| `trigger` | See *Trigger component*. |
| `behavior` | See *Enemy behavior component*. |

### Transform

All three fields are `[x, y, z]` arrays. Rotation is Euler radians (roll/pitch/yaw apply in that order via `XMMatrixRotationRollPitchYaw`).

| Field | Default |
|---|---|
| `position` | `[0, 0, 0]` |
| `rotation` | `[0, 0, 0]` |
| `scale` | `[1, 1, 1]` |

### Render component

```json
"render": {
  "mesh": "cube",
  "material": {
    "texture": "Assets/Textures/ground_checker.png",
    "tint":    [1.0, 1.0, 1.0],
    "sampler": "wrap_linear"
  }
}
```

| Field | Values | Default |
|---|---|---|
| `mesh` | `"cube"`, `"plane"` | `"cube"` |
| `meshPath` | Path to a glTF file (Stage 1 imports). When present, takes precedence over `mesh`. | (none) |
| `material.texture` | Path to texture (WIC-supported format). Omit for untextured. | (none) |
| `material.tint` | `[r, g, b]` in 0..1 linear space | `[1, 1, 1]` |
| `material.sampler` | `"wrap_linear"`, `"clamp_linear"`, `"wrap_point"` | `"wrap_linear"` |

#### External meshes (`meshPath`)

When the scene-object's render block carries `meshPath`, the renderer loads the referenced glTF file via `MeshImporter` and caches the resulting vertex/index buffers per path (same lifetime as the texture cache — survives scene reloads). The `mesh` enum is ignored in this case.

v1 glTF subset accepted by `MeshImporter`:

- glTF 2.0 JSON only (no `.glb` binary container yet).
- Single buffer with a `data:application/octet-stream;base64,...` URI. External `.bin` references are not supported — keep authored test assets self-contained.
- Single mesh with a single primitive, mode `4` (TRIANGLES).
- Attributes: `POSITION` (required, FLOAT VEC3), `NORMAL` and `TEXCOORD_0` (optional, FLOAT VEC3 / VEC2).
- Indices: UNSIGNED_SHORT only (≤ 65535 vertices). UNSIGNED_INT rejects with a clear error at load.

Anything outside this subset rejects at startup via `SceneLoader::Validate`'s asset-sweep path.

Stage 2 (skinned mesh + animation) will widen the subset and most likely vendor `cgltf` to handle skin matrices, animation channels, and `.glb`. Today's parser is documented as the v1 throwaway in `MeshImporter.h`.

### Collision component

XZ axis-aligned box used by the gameplay-level `CollisionSystem`.

| Field | Values | Default |
|---|---|---|
| `halfExtents` | `[halfX, halfZ]` | `[0.5, 0.5]` |
| `blocking` | `true`/`false`. `true` blocks movement; `false` makes the collider non-solid. | `true` |

### Combat component

| Field | Values | Default |
|---|---|---|
| `faction` | `"player"`, `"enemy"`, `"neutral"` | `"neutral"` |
| `maxHp` | Integer | `1` |
| `currentHp` | Integer | `1` |

### Enemy behavior component

Picks the AI class that drives an enemy-faction `SceneObject`. Absent on player and neutral objects; absent on enemies means `"scout"` (the chase + melee default). `SimpleEnemyController` dispatches off `type` each tick.

```json
"behavior": { "type": "archer" }
```

| Field | Values | Default |
|---|---|---|
| `type` | `"scout"` (chase + melee), `"archer"` (stationary hitscan) | `"scout"` |

Validator rejects any other type with a path-prefixed error before the window is created. Adding a new behavior is: extend the allow-list in `SceneLoader.cpp::IsKnownEnemyBehavior`, add a dispatch case to `SimpleEnemyController::Update`, and define the behavior class in `Game/NPC/`.

### Trigger component

Invisible axis-aligned XZ volume that fires the first time the player overlaps it. No rendering or collision required on the same object. `halfExtents` array is `[halfX, halfZ]` — the same packing `CollisionComponent` uses.

| Field | Values | Default |
|---|---|---|
| `halfExtents` | `[halfX, halfZ]` | `[1.0, 1.0]` |
| `tag` | Free-form string; appears in the `[Trigger] Player entered '<tag>'` log and as payload `a` of the `TriggerFired` event. | `""` |
| `fireOnce` | `true` = one-shot; `false` = fire on every entry. Exit detection is not implemented yet. | `true` |
| `action` | Optional `{ "type": ..., "param": ... }` describing what firing should do. See *Trigger action*. Absent = implicit `"log"`. | (none) |

#### Trigger action

```json
"trigger": {
  "halfExtents": [1.5, 1.5],
  "tag": "center_zone",
  "fireOnce": true,
  "action": { "type": "load_scene", "param": "Assets/Scenes/arena_trial.json" }
}
```

| `type` | Effect | `param` meaning |
|---|---|---|
| `"log"` | Prints `[Trigger] Player entered '<tag>'` and publishes `TriggerFired` with `b = <object name>`. Identical to omitting `action`. | ignored |
| `"publish"` | Same log line, plus `TriggerFired` with `b = param`. Use when downstream consumers need scene-chosen payload instead of the object name. | free-form payload string |
| `"load_scene"` | Publishes `LoadSceneRequested { a = param }`. `GameplaySimulation` reloads after the current tick finishes; the scene-graph swap is atomic from the systems' point of view. | path to the destination scene JSON |

Validator rejects any other type with a path-prefixed error before the window is created.

## Prefab merge

A scene object with a `"prefab"` key resolves the referenced JSON once (cached per `SceneLoader::Load` call) and merges it into the scene entry using **component-level shallow override**:

1. Each **top-level key** (`transform`, `render`, `collision`, `combat`, `trigger`, ...) from the prefab is copied onto the scene entry **only if the scene entry does not already contain that key**.
2. `name` and `prefab` in the prefab JSON are ignored — the scene's `name` is authoritative, and nested prefab references are not resolved.
3. No deeper merging happens. If the scene entry has `"render": { "material": { "tint": [...] } }` and the prefab has `"render": { "mesh": "cube", ... }`, the scene's `render` wins entirely — the prefab's `mesh` is **not** inherited. If you need to preserve a prefab field, omit the corresponding top-level key from the scene entry.

### Prefab file contents

Prefab JSON is the *body* of a scene object. No `schemaVersion` key, no outer `scene`/`objects` wrapping.

```json
{
  "render":    { "mesh": "cube", "material": { "tint": [0.92, 0.36, 0.36] } },
  "collision": { "halfExtents": [0.5, 0.5], "blocking": true },
  "combat":    { "faction": "enemy", "maxHp": 3, "currentHp": 3 }
}
```

### What prefabs do and don't help with

Prefabs are worth making when the **same component block repeats verbatim** across scenes or objects. `ArenaWall.prefab.json` is a good example: every wall uses the same `render.mesh` + `render.material.tint` while the `transform.scale` and `collision.halfExtents` differ per wall, so the prefab carries only `render`.

If every field of a component differs per instance, the prefab cannot usefully store it — under shallow override, any scene-side `collision` block replaces the prefab's `collision` entirely, including `blocking`. When that's a problem the fix is not to make the scene omit `collision` (the data is required); it is to either (a) accept the duplication or (b) escalate to deep merge. Deep merge is a Phase 6+ decision, only worthwhile once five or more prefabs share partially-overridden components.

## Scene reload semantics

A scene transition — whether triggered by startup, `--scene`, or a `load_scene` action — is a **full reset**. `Scene::Clear` discards every object (so trigger `fired` flags disappear with them), `ObjectiveSystem::Reset` drops objective progress, and `EventBus` is implicitly empty at the tick boundary where reload runs. There is no player-state carry-over: HP, position, and attack cooldown all come from the new scene's `Player` object.

If later phases need persistence across transitions (save state, inventory, "continue" between hub and arena), the integration point is `GameplaySimulation::ReloadScene` — it is the single call that performs BuildArena + ObjectiveSystem::Reset, so capturing/restoring state around it covers every reload path.

Reload is deferred to **after** `GameplaySimulation::Update` returns. Systems never see a mid-tick scene swap, so iterators over `Scene::GetObjects()` stay valid for the duration of the tick that published `LoadSceneRequested`.

## Versioning policy

- `1`: original schema. No `prefab`, no `trigger`, no `objective`. Still accepted by the v2 loader without modification.
- `2`: adds optional `prefab`/`trigger` scene-level structures (Phase 5), optional top-level `objective` block plus optional `trigger.action` (Phase 6), optional `count` / `any`+`anyOf` shapes inside `objective.conditions` (Phase 7 first slice), and optional `behavior` per scene-object (archer enemy slice). All additions are structurally optional — a v2 scene that uses none of them is indistinguishable from a v1 scene, and v1 scenes load unchanged.

When a breaking change is introduced, bump `schemaVersion` and make the loader accept the previous version's structure verbatim while rejecting future versions with a clear error.
