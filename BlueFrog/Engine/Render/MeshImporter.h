#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// std::uint16_t / std::uint32_t are referenced below via cstdint above.

// Imported mesh in the engine's interleave-friendly form. The renderer copies
// these streams into a LitVertex (or SkinnedVertex when `IsSkinned()`) array;
// consumers outside the renderer that need different layouts can transform
// from here.
struct ImportedMesh
{
	std::vector<float>          positions; // x,y,z stride 3
	std::vector<float>          normals;   // x,y,z stride 3 (optional; empty = generate flat per-tri)
	std::vector<float>          uvs;       // u,v   stride 2 (optional; empty = (0,0))
	std::vector<std::uint16_t>  indices;

	// Skin data — present when both jointIndices and jointWeights are
	// non-empty. cgltf gives us up to 4 influences per vertex (vec4 of
	// joint indices + matching weights); meshes with more influences get
	// truncated by the asset pipeline before reaching us.
	std::vector<std::uint16_t>  jointIndices;        // stride 4
	std::vector<float>          jointWeights;        // stride 4
	std::vector<float>          inverseBindMatrices; // stride 16 (column-major mat4)
	std::uint32_t               jointCount = 0;

	bool IsSkinned() const noexcept
	{
		return !jointIndices.empty() && !jointWeights.empty() && jointCount > 0;
	}
};

// glTF 2.0 mesh importer, backed by cgltf v1.15 (vendored under
// `vendor/cgltf/`). Replaced the hand-written subset parser when Stage 2
// arrived — cgltf carries the weight of skin matrices, animation channels,
// sparse accessors, .glb binary containers, and edge cases we'd otherwise
// have to reinvent.
//
// What still applies at Stage 1 (this header):
//
//   - We still consume one mesh / one primitive per file. Multi-mesh
//     authoring is a content-pipeline question we'll revisit when we have
//     real DCC export needs.
//   - mode = TRIANGLES only.
//   - Attributes parsed: POSITION (required), NORMAL (optional),
//     TEXCOORD_0 (optional). cgltf normalizes / converts so the source
//     accessor's component type doesn't matter from our side.
//   - Indices flatten to uint16 — cgltf upcasts/downcasts as needed but
//     `ImportedMesh::indices` is still uint16 so meshes > 65535 vertices
//     reject with a clear error.
//   - Both embedded `data:` URIs (our hand-authored tetrahedron) and
//     external `.bin` references (Khronos sample assets) work — cgltf
//     resolves both via `cgltf_load_buffers`.
//   - Skin / joint / weight attributes are silently ignored at Stage 1
//     and consumed at Stage 2 once we add the SkinnedVertex format and
//     skinning shader pipeline.
//
// Returns true on success. On failure errorOut (if non-null) is filled with
// a path-prefixed message matching the SceneLoader error style.
namespace MeshImporter
{
	bool Load(const std::filesystem::path& gltfPath, ImportedMesh& out, std::string* errorOut) noexcept;
}
