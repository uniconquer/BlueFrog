# Universal Characters

Source: Quaternius — "Universal Base Characters" + "Universal Animation Library" (quaternius.com, CC0 / royalty-free).

`HeroMale/HeroMale.gltf` is a build artifact: the Superhero_Male base mesh merged
with UAL1 clips in Blender (`_tmp_fbximport/merge_universal.py`), with clips
renamed to the engine's expected names (Idle / Walk / Run / Die / Slash /
SlashDown / Hit / Ride) and all mesh objects joined into a single glTF mesh —
the engine's MeshImporter only reads `meshes[0]` (merging its primitives), so
body/eyes/eyebrows must live in one mesh.
