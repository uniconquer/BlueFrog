# BrainStem sample asset

`BrainStem.gltf` (glTF-Embedded variant) fetched from the Khronos Group
glTF Sample Assets repository:

- Source: https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/BrainStem
- Variant: `glTF-Embedded` (single file with base64-embedded buffer + image)
- License: see upstream per-model `README.md`. The repo aggregates under
  `LicenseRef-glTF-Sample-Assets` permitting use, modification, and
  redistribution including in commercial software, with attribution.

Used at Phase F Stage 4c-2-ext to replace the EnemyArcher cube with a
distinct humanoid character (alien biped with a single walk-cycle
animation). Single mesh / single primitive / 18 joints — well within
our v1 importer subset. Embedded JPEG baseColorTexture exercises the
base64-data-URI path landed in Stage 4c-3.
