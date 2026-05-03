# Fox sample asset

`Fox.gltf` + `Fox.bin` (separate-files glTF variant) fetched from the Khronos
Group glTF Sample Assets repository:

- Source: https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/Fox
- Variant: `glTF` (external .bin, no embedded base64). The repo does not
  ship a `glTF-Embedded` variant for this asset, so we vendor both files;
  cgltf's `cgltf_load_buffers` resolves `.bin` URIs relative to the
  `.gltf` path automatically.
- License: CC0 1.0 Universal — public domain. Original author Pixar
  (donated to the glTF working group); upstream repo aggregates under
  `LicenseRef-glTF-Sample-Assets`.

Used at Phase F Stage 4c-2 to replace the EnemyScout cube with a real
creature mesh. Three animations (Survey, Walk, Run) make this the first
asset in the project that exercises the multi-clip path landed in
Stage 4a (Stage 4b's state machine maps gameplay state to clip name).
Single mesh, single primitive — fits our v1 importer subset.

**Author scale note:** Fox is authored at glTF's default scale, but its
modeled extent is large (~100 units tall in source space). Scenes
referencing the prefab apply a downscale on the SceneObject transform
(typically `scale: 0.04`) to land at humanoid-eye-level enemy size.
