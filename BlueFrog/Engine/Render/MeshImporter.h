#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// Imported mesh in the engine's interleave-friendly form. The renderer copies
// these streams into a LitVertex array; consumers outside the renderer that
// need different layouts can transform from here.
struct ImportedMesh
{
	std::vector<float>          positions; // x,y,z stride 3
	std::vector<float>          normals;   // x,y,z stride 3 (optional; empty = generate flat per-tri)
	std::vector<float>          uvs;       // u,v   stride 2 (optional; empty = (0,0))
	std::vector<std::uint16_t>  indices;
};

// Minimal glTF 2.0 subset loader. Targets the assets BlueFrog actually
// authors and ships at Stage 1 of the rendering climb (static meshes only).
//
// **Future migration note:** when Stage 2 adds skinned-mesh + animation
// support, vendor `cgltf.h` (single-header, MIT, well-maintained) and
// replace this parser. cgltf handles skin matrices, animation channels,
// sparse accessors, and .glb binary containers that this v1 parser does
// not. The current ~100-line implementation is right-sized for the
// "decode one base64 buffer into a vertex stream" job and not worth
// pulling 5000 lines of cgltf for.
//
// v1 supports:
//
//   - Single buffer with a `data:` URI (base64). External .bin files are NOT
//     supported in v1 — keep test assets self-contained.
//   - Single mesh with a single primitive.
//   - mode = 4 (TRIANGLES) only.
//   - Attributes: POSITION (required), NORMAL (optional), TEXCOORD_0
//     (optional). All FLOAT (componentType 5126).
//   - Indices: UNSIGNED_SHORT (5123) only. Imported meshes must have < 65536
//     vertices. Validator rejects UNSIGNED_INT with a clear error.
//   - Accessors must have byteOffset 0 within their bufferView (no
//     interleaved attributes — the parser walks one attribute at a time).
//
// Returns true on success. On failure errorOut (if non-null) is filled with
// a path-prefixed message matching the SceneLoader error style.
namespace MeshImporter
{
	bool Load(const std::filesystem::path& gltfPath, ImportedMesh& out, std::string* errorOut) noexcept;
}
