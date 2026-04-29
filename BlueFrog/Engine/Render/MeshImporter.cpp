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
			default: break; // joints / weights etc. ignored until Stage 2
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

		cgltf_free(data);
		return true;
	}
}
