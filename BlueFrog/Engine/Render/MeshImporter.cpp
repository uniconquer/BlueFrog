#include "MeshImporter.h"

#include <cgltf/cgltf.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace
{
	std::string PathPrefix(const std::filesystem::path& p)
	{
		return p.string() + ": ";
	}

	bool SetError(std::string* out, std::string msg)
	{
		if (out) *out = std::move(msg);
		return false;
	}

	const char* CgltfResultName(cgltf_result r)
	{
		switch (r)
		{
		case cgltf_result_success:        return "success";
		case cgltf_result_data_too_short: return "data_too_short";
		case cgltf_result_unknown_format: return "unknown_format";
		case cgltf_result_invalid_json:   return "invalid_json";
		case cgltf_result_invalid_gltf:   return "invalid_gltf";
		case cgltf_result_invalid_options:return "invalid_options";
		case cgltf_result_file_not_found: return "file_not_found";
		case cgltf_result_io_error:       return "io_error";
		case cgltf_result_out_of_memory:  return "out_of_memory";
		case cgltf_result_legacy_gltf:    return "legacy_gltf";
		default:                          return "unknown";
		}
	}

	// Returns the number of float components per element in the accessor
	// (1 for SCALAR, 2 VEC2, 3 VEC3, 4 VEC4, etc.).
	cgltf_size ComponentsPerElement(const cgltf_accessor* a)
	{
		switch (a->type)
		{
		case cgltf_type_scalar: return 1;
		case cgltf_type_vec2:   return 2;
		case cgltf_type_vec3:   return 3;
		case cgltf_type_vec4:   return 4;
		case cgltf_type_mat2:   return 4;
		case cgltf_type_mat3:   return 9;
		case cgltf_type_mat4:   return 16;
		default:                return 0;
		}
	}

	// Pulls a float-typed vertex attribute out of cgltf into a flat
	// vector<float>. Used for POSITION (vec3), NORMAL (vec3), TEXCOORD_0
	// (vec2). cgltf_accessor_unpack_floats handles type/normalization
	// conversion automatically — including the rare case where a glTF
	// emits half-precision or normalized integer attribute values.
	bool UnpackFloatAttribute(const cgltf_accessor* a, std::vector<float>& out, std::string* errorOut, const std::string& prefix, const char* attrName)
	{
		if (a == nullptr) return true; // optional attribute, no-op
		const cgltf_size components = ComponentsPerElement(a);
		if (components == 0)
		{
			return SetError(errorOut, prefix + "attribute '" + attrName + "' has unsupported accessor type");
		}
		const cgltf_size totalFloats = a->count * components;
		out.resize(totalFloats);
		const cgltf_size unpacked = cgltf_accessor_unpack_floats(a, out.data(), totalFloats);
		if (unpacked != totalFloats)
		{
			return SetError(errorOut, prefix + "attribute '" + attrName + "' unpack failed (got " + std::to_string(unpacked) + "/" + std::to_string(totalFloats) + " floats)");
		}
		return true;
	}
}

namespace MeshImporter
{
	bool Load(const std::filesystem::path& gltfPath, ImportedMesh& out, std::string* errorOut) noexcept
	{
		const std::string prefix = PathPrefix(gltfPath);
		const std::string pathStr = gltfPath.string();
		out = {};

		cgltf_options options = {};
		cgltf_data* data = nullptr;

		cgltf_result r = cgltf_parse_file(&options, pathStr.c_str(), &data);
		if (r != cgltf_result_success)
		{
			return SetError(errorOut, prefix + "cgltf_parse_file failed: " + CgltfResultName(r));
		}

		// load_buffers handles both embedded data: URIs (our hand-authored
		// tetrahedron) and external .bin files (Khronos sample assets).
		// The base path it walks for relative .bin lookup is the .gltf
		// location, which is exactly what the URI resolution expects.
		r = cgltf_load_buffers(&options, data, pathStr.c_str());
		if (r != cgltf_result_success)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "cgltf_load_buffers failed: " + CgltfResultName(r));
		}

		// v1 still consumes one mesh / one primitive. Multi-mesh / multi-
		// primitive support is a Stage 2+ extension once we have a real
		// authoring pipeline producing such files.
		if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "no meshes/primitives in glTF");
		}
		const cgltf_mesh& mesh = data->meshes[0];
		const cgltf_primitive& prim = mesh.primitives[0];
		if (prim.type != cgltf_primitive_type_triangles)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "primitive type must be TRIANGLES");
		}

		// Walk attributes once, picking out the channels we care about.
		const cgltf_accessor* accPos = nullptr;
		const cgltf_accessor* accNor = nullptr;
		const cgltf_accessor* accUv  = nullptr;
		const cgltf_accessor* accJoints = nullptr;
		const cgltf_accessor* accWeights = nullptr;
		for (cgltf_size i = 0; i < prim.attributes_count; ++i)
		{
			const cgltf_attribute& a = prim.attributes[i];
			switch (a.type)
			{
			case cgltf_attribute_type_position: accPos = a.data; break;
			case cgltf_attribute_type_normal:   accNor = a.data; break;
			case cgltf_attribute_type_texcoord:
				if (a.index == 0) accUv = a.data; // only TEXCOORD_0 in v1
				break;
			case cgltf_attribute_type_joints:
				if (a.index == 0) accJoints = a.data; // only JOINTS_0 — Stage 2 supports 4 influences
				break;
			case cgltf_attribute_type_weights:
				if (a.index == 0) accWeights = a.data; // only WEIGHTS_0
				break;
			default: break;
			}
		}

		if (accPos == nullptr)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "primitive missing required POSITION attribute");
		}

		if (!UnpackFloatAttribute(accPos, out.positions, errorOut, prefix, "POSITION") ||
			!UnpackFloatAttribute(accNor, out.normals,   errorOut, prefix, "NORMAL")   ||
			!UnpackFloatAttribute(accUv,  out.uvs,       errorOut, prefix, "TEXCOORD_0"))
		{
			cgltf_free(data);
			return false;
		}

		// Indices. cgltf_accessor_unpack_indices converts to whatever output
		// width we ask for. Stage 1 keeps the ImportedMesh::indices vector
		// at uint16, so anything > 65535 vertices rejects here — matching
		// the contract the renderer's MeshBuffers/IndexBuffer enforces.
		if (prim.indices == nullptr)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "primitive missing index buffer");
		}
		const cgltf_size vertexCount = out.positions.size() / 3;
		if (vertexCount > 65535)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "mesh has " + std::to_string(vertexCount) + " vertices (max 65535 in v1, 16-bit index buffer)");
		}
		out.indices.resize(prim.indices->count);
		const cgltf_size unpacked = cgltf_accessor_unpack_indices(prim.indices, out.indices.data(), sizeof(std::uint16_t), prim.indices->count);
		if (unpacked != prim.indices->count)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "index unpack failed (" + std::to_string(unpacked) + "/" + std::to_string(prim.indices->count) + ")");
		}

		// Sanity: vertex count consistency.
		if (vertexCount == 0)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "primitive has zero vertices");
		}
		if (!out.normals.empty() && out.normals.size() / 3 != vertexCount)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "NORMAL count does not match POSITION count");
		}
		if (!out.uvs.empty() && out.uvs.size() / 2 != vertexCount)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "TEXCOORD_0 count does not match POSITION count");
		}

		// Skin data extraction (Stage 2). Both JOINTS_0 and WEIGHTS_0 must
		// be present, plus a node referencing the mesh that points at a
		// skin block carrying inverse bind matrices. If any piece is
		// missing we treat the mesh as static (existing Stage 1 behavior).
		const cgltf_skin* skin = nullptr;
		for (cgltf_size i = 0; i < data->nodes_count && skin == nullptr; ++i)
		{
			const cgltf_node& n = data->nodes[i];
			if (n.mesh == &mesh && n.skin != nullptr)
			{
				skin = n.skin;
			}
		}

		if (accJoints && accWeights && skin && skin->joints_count > 0)
		{
			// Joints: read as uint, stride 4. cgltf's read_uint handles the
			// underlying UNSIGNED_BYTE / UNSIGNED_SHORT representations
			// uniformly so we don't have to branch on accessor componentType.
			out.jointIndices.resize(vertexCount * 4);
			for (cgltf_size i = 0; i < vertexCount; ++i)
			{
				cgltf_uint tmp[4] = {};
				if (!cgltf_accessor_read_uint(accJoints, i, tmp, 4))
				{
					cgltf_free(data);
					return SetError(errorOut, prefix + "JOINTS_0 read failed at vertex " + std::to_string(i));
				}
				for (int k = 0; k < 4; ++k)
				{
					out.jointIndices[i * 4 + k] = static_cast<std::uint16_t>(tmp[k]);
				}
			}

			// Weights: float vec4 stream.
			if (!UnpackFloatAttribute(accWeights, out.jointWeights, errorOut, prefix, "WEIGHTS_0"))
			{
				cgltf_free(data);
				return false;
			}
			if (out.jointWeights.size() / 4 != vertexCount)
			{
				cgltf_free(data);
				return SetError(errorOut, prefix + "WEIGHTS_0 count does not match POSITION count");
			}

			// Inverse bind matrices: mat4 array, one per joint. cgltf gives
			// them as 16 floats in column-major order — DirectXMath's
			// XMMatrix... functions interpret memory as row-major, so the
			// renderer is responsible for the transpose when uploading to
			// the cbuffer (this layer just stores the raw glTF stream).
			if (skin->inverse_bind_matrices)
			{
				const cgltf_size jointCount = skin->joints_count;
				if (skin->inverse_bind_matrices->count != jointCount)
				{
					cgltf_free(data);
					return SetError(errorOut, prefix + "skin.inverseBindMatrices.count != joints.count");
				}
				out.inverseBindMatrices.resize(jointCount * 16);
				const cgltf_size unpackedIBM = cgltf_accessor_unpack_floats(
					skin->inverse_bind_matrices,
					out.inverseBindMatrices.data(),
					jointCount * 16);
				if (unpackedIBM != jointCount * 16)
				{
					cgltf_free(data);
					return SetError(errorOut, prefix + "inverseBindMatrices unpack failed");
				}
				out.jointCount = static_cast<std::uint32_t>(jointCount);
			}
			else
			{
				// glTF allows omitting IBMs (defaults to identity per joint),
				// but our renderer wants explicit matrices. Stage 2 rejects.
				cgltf_free(data);
				return SetError(errorOut, prefix + "skin.inverseBindMatrices is required (Stage 2 does not synthesize identity defaults)");
			}
		}

		cgltf_free(data);
		return true;
	}
}
