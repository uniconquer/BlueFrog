# BlueFrog Scene JSON Schema (v2)

This document describes the on-disk format for scenes and prefabs used by `SceneLoader` and `PrefabLoader`. The schema is deliberately small; extend it only when a real consumer exists.

- Scene files live in `BlueFrog/Assets/Scenes/*.json`.
- Prefab files live in `BlueFrog/Assets/Prefabs/*.prefab.json`.
- Relative paths are resolved against the **current working directory** at launch (typically `$(ProjectDir)` = `BlueFrog/BlueFrog/` under Visual Studio). Move the CWD and scene/prefab references break the same way.
- Both loaders use [`nlohmann/json`](https://github.com/nlohmann/json); trailing commas, comments, and other JSON5 features are not supported.

## Top-level scene

```json
{
  "schemaVersion": 2,
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
| `scene.name` | no | Informational. Not used at runtime. |
| `scene.camera.target` | no | `[x, y, z]` world-space point the camera follows on spawn. Defaults to the engine's existing camera state if omitted. |
| `scene.objects` | no | Array. Missing array = empty scene. |

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
  "trigger":   { "halfExtents": [1.5, 1.5], "tag": "center_zone", "fireOnce": true }
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
| `material.texture` | Path to texture (WIC-supported format). Omit for untextured. | (none) |
| `material.tint` | `[r, g, b]` in 0..1 linear space | `[1, 1, 1]` |
| `material.sampler` | `"wrap_linear"`, `"clamp_linear"`, `"wrap_point"` | `"wrap_linear"` |

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

### Trigger component

Invisible axis-aligned XZ volume that fires a log line the first time the player overlaps it. No rendering or collision required on the same object. `halfExtents` array is `[halfX, halfZ]` — the same packing `CollisionComponent` uses.

| Field | Values | Default |
|---|---|---|
| `halfExtents` | `[halfX, halfZ]` | `[1.0, 1.0]` |
| `tag` | Free-form string; appears in the `[Trigger] Player entered '<tag>'` log. | `""` |
| `fireOnce` | `true` = one-shot; `false` = fire on every entry. Exit detection is not implemented yet. | `true` |

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

## Versioning policy

- `1`: original schema. No `prefab`, no `trigger`. Still accepted by the v2 loader without modification.
- `2`: adds optional `prefab` scene-object key and optional `trigger` component. v1 scenes are structurally a subset — no migration needed.

When a breaking change is introduced, bump `schemaVersion` and make the loader accept the previous version's structure verbatim while rejecting future versions with a clear error.
