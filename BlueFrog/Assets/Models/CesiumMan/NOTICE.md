# CesiumMan sample asset

`CesiumMan.gltf` (glTF-Embedded variant) was fetched from the Khronos Group
glTF Sample Assets repository:

- Source: https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/CesiumMan
- Variant: `glTF-Embedded` (single file with base64-embedded buffers)
- License: see the upstream repo's per-model `README.md` for the specific
  license (the asset itself was originally produced by Cesium under
  CC-BY 4.0; the upstream repo aggregates it under
  `LicenseRef-glTF-Sample-Assets` permitting use, modification, and
  redistribution including in commercial software, with attribution).

Used at Phase F Stage 4c to replace the player cube with a real humanoid
character mesh. Single mesh / single primitive / one animation clip
("walk" cycle) — matches our importer's v1 subset and exercises the
per-instance animation state plumbing landed in Stage 4a.
