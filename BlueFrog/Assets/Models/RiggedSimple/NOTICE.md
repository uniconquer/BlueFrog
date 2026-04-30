# RiggedSimple sample asset

`RiggedSimple.gltf` (glTF-Embedded variant) was fetched from the Khronos Group
glTF Sample Assets repository:

- Source: https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/RiggedSimple
- Variant: `glTF-Embedded` (single file with base64-embedded buffers)
- License: see the upstream repo's per-model `README.md` for the specific
  license that applies. The repo as a whole uses the SPDX
  `LicenseRef-glTF-Sample-Assets` license, which permits use, modification,
  and redistribution including in commercial software, with attribution.

Used at Phase F Stage 2 as the canonical "smallest possible skinned mesh"
test asset. Two boxes connected by a single joint — minimum surface area to
exercise JOINTS_0 / WEIGHTS_0 / inverseBindMatrices import and the skinned
shader pipeline without dragging in a full character rig.
