#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// std::uint16_t / std::uint32_t are referenced below via cstdint above.

// One animation channel: a stream of (time, value) keyframes targeting a
// specific joint's translation / rotation / scale component. v1 supports
// LINEAR + STEP interpolation; CUBICSPLINE is rejected at import.
struct ImportedAnimationChannel
{
	enum class Path { Translation, Rotation, Scale };
	enum class Interpolation { Linear, Step };

	int                targetJoint   = -1;          // index into the skin's joints array
	Path               path          = Path::Translation;
	Interpolation      interpolation = Interpolation::Linear;
	std::vector<float> times;                       // count = N keyframes
	std::vector<float> values;                      // stride 3 (T/S) or 4 (R quaternion xyzw)
};

// One animation clip. v1 ImportedMesh carries the *first* clip in the file —
// multi-clip support comes when the gameplay layer needs it.
struct ImportedAnimation
{
	std::string                            name;
	float                                  duration = 0.0f; // max channel time
	std::vector<ImportedAnimationChannel>  channels;
};

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

	// Joint hierarchy — index of the parent joint in the same `joints` array,
	// or -1 if the joint has no parent inside the skin (i.e., its parent is
	// outside the skin's joint set, treated as the skin root). Stored in the
	// same order as cgltf's `skin->joints[]`, which we trust to be
	// topologically sorted (parents before children) — true for every
	// Khronos sample asset; if a future asset violates this we'll surface it.
	std::vector<int>            jointParents;

	// Bind-pose local TRS per joint. Renderer composes these per frame as
	// the *fallback* values for joints whose components an animation does
	// not override.
	std::vector<float>          jointBindTranslation; // stride 3
	std::vector<float>          jointBindRotation;    // stride 4 (quaternion xyzw)
	std::vector<float>          jointBindScale;       // stride 3

	// First animation clip in the file (Stage 3). Empty `channels` means
	// no animation — Renderer falls back to bind-pose joint matrices.
	ImportedAnimation           animation;

	bool IsSkinned() const noexcept
	{
		return !jointIndices.empty() && !jointWeights.empty() && jointCount > 0;
	}

	bool HasAnimation() const noexcept
	{
		return !animation.channels.empty() && animation.duration > 0.0f;
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
