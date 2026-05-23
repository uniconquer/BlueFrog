# Units and Scales

## The convention

**1 BlueFrog unit = 1 meter.** Everything else in the engine, game code, and asset pipeline assumes this.

This matches Unity, Godot, the glTF 2.0 specification, and Blender's default. Unreal famously uses 1 cm — we don't. A `position.y = 1.7` means 1.7 meters off the ground.

Why a fixed unit at all: external modelers, physics defaults, character locomotion speeds, and field-of-view math all only make sense once you commit to a meaning for the number `1`. Pick one and document it.

## What the convention covers

| Where | Interpretation |
|---|---|
| `Transform.position` (XYZ) | World-space meters |
| `Transform.scale` | Multiplier on a 1-m source (i.e. `1.0` is "natural size") |
| `CollisionComponent.halfExtents` (X, Z) | Meters, half-axis |
| `TriggerComponent.halfExtents` | Meters |
| Camera distances, FOV math | Meters |
| Player / enemy `attackRange`, `attackSpeed`, `dashSpeed` etc. | `attackRange = 2.4` ⇒ 2.4 m. `moveSpeed = 6.5` ⇒ 6.5 m/s. |
| Knockback impulse | Meters per second (`KnockbackSpeed = 4.0`) |
| `DamagePopup.kFloatUpDip` | DIPs (screen-space). Not meters — that one's UI. |

Anything authored in points/DIPs (`TextLayout::*`, `UiLayout::*`) is **explicitly UI**, not world space. Don't confuse the two.

## The asset import problem

glTF 2.0 spec says "all linear distances are in meters" but Khronos sample models do whatever they want:

| Sample | Source size | Should be | importScale | Real in-game size (`importScale × transform.scale`) |
|---|---|---|---|---|
| `CesiumMan.gltf` | ~1.2 m | adult human ~1.7 m | `1.4` | 1.7 m |
| `Fox.gltf` | ~70 m (!) | medium animal | `0.04` (scout) / `0.1` (boss) | 2.8 m / 7 m |
| `BrainStem.gltf` | ~0.45 m | small humanoid | `1.0` | 0.45 m (kept small for archer-feel) |
| `RiggedSimple.gltf` | ~0.4 m | tech-demo prop | `1.0` | 0.2 m (scaled down in scene for "decorative") |

Without `importScale`, the alternative is hand-tuning `transform.scale` on every instance of an asset in every scene — leaking authoring detail into scene files and bloating diffs whenever you swap a model.

## `RenderComponent.importScale`

The fix. Authored at the **prefab** level (not the scene level), exactly once per source mesh:

```json
// Player.prefab.json
{
  "render": {
    "meshPath": "Assets/Models/CesiumMan/CesiumMan.gltf",
    "importScale": 1.4
  },
  "collision": { "halfExtents": [0.45, 0.45], "blocking": true },
  ...
}
```

The renderer multiplies `importScale × transform.scale` into the model matrix at draw time, so the visible result is identical to "scale = 1.4 everywhere", but `transform.scale` now stays meaningful as **intentional in-game size**:

- `Elder` in `village.json` uses `transform.scale = 1.0` → reads as "default-sized adult villager", in meters, full stop.
- `Lina` uses `transform.scale = 0.8` → "noticeably smaller, like a child", in meters.
- An imagined boss instance with `transform.scale = 2.0` → "2× a default adult".

The `importScale` is invisible from the scene file's point of view, which is what you want when the scene author isn't the asset author.

### Important: collision is not auto-scaled

`importScale` only rescales the visual mesh. `CollisionComponent.halfExtents` is authored directly in meters at the prefab level, so it doesn't accidentally change when you tweak `importScale`. If you change a model's `importScale`, eyeball the collision box (F1 debug gizmos) and update `halfExtents` to match.

## Workflow for adding a new external model

1. Drop the `.gltf` (+ `.bin` / textures) under `BlueFrog/Assets/Models/<ModelName>/`.
2. Eyeball the source size — open the glTF in Blender or [gltf.report](https://gltf.report/) and read the bounding-box dimensions. Note the largest axis.
3. Decide the target in-game height in meters (use the table below).
4. `importScale = target / source`. E.g. source is 50 m, target is 2.5 m → `importScale = 0.05`.
5. Create the prefab with `"importScale": <value>`.
6. Set `CollisionComponent.halfExtents` directly in meters — start with ~half the bounding box on X/Z.
7. Add an instance in a scene with `transform.scale = 1.0` and check it in-game with F1 (debug gizmos) and F2 (inspector). Tweak `importScale` if the size is off.

### Blender export checklist

When exporting glTF from Blender:

- **Unit Scale = 1.0** (Scene Properties → Units → Unit Scale).
- **Apply transforms** before export, or check the "Apply Modifiers" export option.
- **Length = Meters** in scene units.
- Export glTF with **+Y up** (BlueFrog convention; the engine's camera and gravity assume Y up).
- Animations: don't bake at insane sample rates — 30 fps is plenty for clip playback.

Done this way, your `importScale` should be ~1.0 and you can skip authoring the field entirely.

## Character size guide

Use this as the default authoring intent. Always meters; multiply by `importScale × transform.scale` at draw time.

| Type | Height (m) | Notes |
|---|---|---|
| Adult human | 1.7 | Default villager / player baseline. Set `transform.scale = 1.0` for one of these. |
| Child / small villager | 1.1 – 1.3 | `transform.scale ≈ 0.7` if prefab is adult-sized. |
| Small enemy (scout-class) | 1.5 – 2.0 | Quick, aggressive feel. |
| Medium enemy (brute-class) | 2.5 – 3.5 | Slow, weighty. |
| Boss (cinematic) | 4.0 – 7.0 | Has to feel like an event. |
| Hut / market stall | 2.5 – 3.5 (tall axis) | Walkable space inside is overhead. |
| Two-story building | 5.0 – 7.0 | Two human-heights stacked + roof. |
| Tree (sapling → mature) | 2.0 – 12.0 | Big spread, art-direction dependent. |

These are **suggested**, not enforced. Drift from them only when you have a reason ("this is a goblin boss meant to feel small but deadly, 1.4 m intentional").

## Pitfalls and tips

- **Don't put `importScale` in scene files.** It's a property of the source asset, not of an instance. Put it in the prefab. Scenes only override it as a last resort (overriding the `render` block via the scene's component-shallow-override semantics will replace the prefab's `importScale` too, so be careful).
- **`scale = [0, 0, 0]` makes things invisible.** The loader treats `importScale ≤ 0` as "use the default 1.0", but `transform.scale` doesn't get that safety net.
- **Negative scale flips winding** — backface culling will eat the mesh. Avoid.
- **Skinned meshes obey `importScale`** the same way static meshes do — the matrix multiplication happens before the per-joint skinning pose math, so the whole rig + animation just rescale uniformly.
- **Camera distances need to scale with your world.** If you make characters 2× as tall, the camera at `radius = 12` might frame them tighter than you want. Update `TopDownCamera`'s defaults if you globally shift character size.

## Future work (not done)

- **Automatic bounding-box probe** during MeshImporter: log a warning when a mesh's bbox falls outside `[0.1, 50]` meters, hinting that `importScale` is probably needed.
- **`transform.scale` per-axis vs uniform**: today every scale is XYZ-separate, but most characters want uniform. A separate `uniformScale` field could simplify auth.
- **Validator check**: forbid `transform.scale` outside `[0.1, 10]` to catch obvious typos (e.g. authoring 0.04 when you meant to use `importScale`).
