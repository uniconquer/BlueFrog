#include "MeshImporter.h"

#include <cgltf/cgltf.h>
#include <DirectXMath.h>

#include <cstdint>
#include <cstring>
#include <fstream>
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

	// Decodes a base64 string into raw bytes. Used for `data:image/...;base64,`
	// image URIs that cgltf parses but does not auto-decode for us. Standard
	// MIME alphabet, ignores whitespace + padding.
	bool DecodeBase64(const char* in, std::size_t inLen, std::vector<std::uint8_t>& out)
	{
		std::int8_t lut[256];
		for (int i = 0; i < 256; ++i) lut[i] = -1;
		for (int i = 0; i < 26; ++i) lut['A' + i] = static_cast<std::int8_t>(i);
		for (int i = 0; i < 26; ++i) lut['a' + i] = static_cast<std::int8_t>(26 + i);
		for (int i = 0; i < 10; ++i) lut['0' + i] = static_cast<std::int8_t>(52 + i);
		lut[static_cast<unsigned>('+')] = 62;
		lut[static_cast<unsigned>('/')] = 63;

		out.clear();
		out.reserve((inLen * 3) / 4);
		int buf = 0;
		int bits = 0;
		for (std::size_t i = 0; i < inLen; ++i)
		{
			const char c = in[i];
			if (c == '=' || c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
			const int v = lut[static_cast<unsigned char>(c)];
			if (v < 0) return false;
			buf = (buf << 6) | v;
			bits += 6;
			if (bits >= 8)
			{
				bits -= 8;
				out.push_back(static_cast<std::uint8_t>((buf >> bits) & 0xFF));
			}
		}
		return true;
	}

	// Resolves a cgltf_image to raw encoded bytes (PNG/JPEG/etc.). Three
	// glTF image storage forms, in priority order:
	//   1. buffer_view: image bytes live inside the .gltf/.bin buffer.
	//   2. data: URI with base64 payload (embedded form).
	//   3. external file URI: load relative to the .gltf path.
	// Returns true on success; on false `out` is undefined and the caller
	// silently treats the mesh as un-textured (no fatal error — missing
	// textures degrade gracefully to flat-shaded white).
	bool ResolveImageBytes(const cgltf_image& img, const std::filesystem::path& gltfPath, std::vector<std::uint8_t>& out)
	{
		// 1. Image stored inside a buffer view.
		if (img.buffer_view != nullptr && img.buffer_view->buffer != nullptr)
		{
			const cgltf_buffer_view* bv = img.buffer_view;
			const std::uint8_t* base = static_cast<const std::uint8_t*>(bv->buffer->data);
			if (base == nullptr) return false;
			const std::size_t offset = bv->offset;
			const std::size_t length = bv->size;
			out.assign(base + offset, base + offset + length);
			return true;
		}

		// 2 / 3: image has a URI.
		if (img.uri == nullptr) return false;
		const std::string uri = img.uri;
		const std::string dataPrefix = "data:";
		if (uri.compare(0, dataPrefix.size(), dataPrefix) == 0)
		{
			const std::size_t b64 = uri.find(";base64,");
			if (b64 == std::string::npos) return false;
			const char* payload = uri.c_str() + b64 + std::strlen(";base64,");
			const std::size_t payloadLen = uri.size() - (b64 + std::strlen(";base64,"));
			return DecodeBase64(payload, payloadLen, out);
		}

		// External file: resolve relative to the .gltf parent directory.
		const std::filesystem::path filePath = gltfPath.parent_path() / uri;
		std::ifstream f(filePath, std::ios::binary);
		if (!f.is_open()) return false;
		f.seekg(0, std::ios::end);
		const std::streamoff len = f.tellg();
		if (len <= 0) return false;
		f.seekg(0, std::ios::beg);
		out.resize(static_cast<std::size_t>(len));
		f.read(reinterpret_cast<char*>(out.data()), len);
		return f.good() || f.eof();
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

		// v1 still consumes one mesh per file but now accepts ANY number of
		// primitives within that mesh — they get merged into a single
		// ImportedMesh stream with per-primitive vertex base offsets
		// applied to indices. This is what makes Mixamo / Sketchfab
		// characters loadable, since those split body/eyes/clothing/etc.
		// into separate primitives. v1 still keeps the single-diffuse-
		// texture limit: first primitive with a material wins.
		if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "no meshes/primitives in glTF");
		}
		const cgltf_mesh& mesh = data->meshes[0];

		// Per-primitive accumulator. We append into out.* streams as we
		// go; uint16 index cap applies to the merged total at the end.
		bool anyHasJoints     = false;
		for (cgltf_size pi = 0; pi < mesh.primitives_count; ++pi)
		{
			const cgltf_primitive& prim = mesh.primitives[pi];
			if (prim.type != cgltf_primitive_type_triangles)
			{
				cgltf_free(data);
				return SetError(errorOut, prefix + "primitive[" + std::to_string(pi) + "] type must be TRIANGLES");
			}

			const cgltf_accessor* accPos     = nullptr;
			const cgltf_accessor* accNor     = nullptr;
			const cgltf_accessor* accUv      = nullptr;
			const cgltf_accessor* accColor   = nullptr;
			const cgltf_accessor* accJoints  = nullptr;
			const cgltf_accessor* accWeights = nullptr;
			for (cgltf_size i = 0; i < prim.attributes_count; ++i)
			{
				const cgltf_attribute& a = prim.attributes[i];
				switch (a.type)
				{
				case cgltf_attribute_type_position: accPos = a.data; break;
				case cgltf_attribute_type_normal:   accNor = a.data; break;
				case cgltf_attribute_type_texcoord: if (a.index == 0) accUv      = a.data; break;
				case cgltf_attribute_type_color:    if (a.index == 0) accColor   = a.data; break;
				case cgltf_attribute_type_joints:   if (a.index == 0) accJoints  = a.data; break;
				case cgltf_attribute_type_weights:  if (a.index == 0) accWeights = a.data; break;
				default: break;
				}
			}

			if (accPos == nullptr)
			{
				cgltf_free(data);
				return SetError(errorOut, prefix + "primitive[" + std::to_string(pi) + "] missing POSITION");
			}

			std::vector<float> tmpPos, tmpNor, tmpUv, tmpColor, tmpWeights;
			if (!UnpackFloatAttribute(accPos,   tmpPos,   errorOut, prefix, "POSITION") ||
				!UnpackFloatAttribute(accNor,   tmpNor,   errorOut, prefix, "NORMAL")   ||
				!UnpackFloatAttribute(accUv,    tmpUv,    errorOut, prefix, "TEXCOORD_0") ||
				!UnpackFloatAttribute(accColor, tmpColor, errorOut, prefix, "COLOR_0"))
			{
				cgltf_free(data);
				return false;
			}
			// COLOR_0 may be vec3 or vec4. We normalize to rgba stride 4.
			const cgltf_size colorComps = accColor ? ComponentsPerElement(accColor) : 0;

			const cgltf_size primVertexCount = tmpPos.size() / 3;
			if (primVertexCount == 0)
			{
				cgltf_free(data);
				return SetError(errorOut, prefix + "primitive[" + std::to_string(pi) + "] has zero vertices");
			}

			const cgltf_size vertexBase = out.positions.size() / 3;
			// uint16 cap: stop if appending this primitive would overflow.
			if (vertexBase + primVertexCount > 65535)
			{
				cgltf_free(data);
				return SetError(errorOut, prefix + "merged mesh exceeds 65535 vertices at primitive[" + std::to_string(pi) + "] (v1 16-bit index buffer)");
			}

			// Position: straight append.
			out.positions.insert(out.positions.end(), tmpPos.begin(), tmpPos.end());

			// Normal: append or pad with (0,1,0) so downstream stride stays consistent.
			if (tmpNor.size() / 3 == primVertexCount)
			{
				out.normals.insert(out.normals.end(), tmpNor.begin(), tmpNor.end());
			}
			else
			{
				for (cgltf_size i = 0; i < primVertexCount; ++i)
				{
					out.normals.push_back(0.0f); out.normals.push_back(1.0f); out.normals.push_back(0.0f);
				}
			}

			// UV: append or pad with (0,0).
			if (tmpUv.size() / 2 == primVertexCount)
			{
				out.uvs.insert(out.uvs.end(), tmpUv.begin(), tmpUv.end());
			}
			else
			{
				for (cgltf_size i = 0; i < primVertexCount; ++i)
				{
					out.uvs.push_back(0.0f); out.uvs.push_back(0.0f);
				}
			}

			// Vertex color: normalize to rgba stride 4. vec3 source gets
			// alpha=1; missing color pads white so an untextured primitive
			// mixed with a colored one in the same mesh still renders.
			if (colorComps >= 3 && tmpColor.size() / colorComps == primVertexCount)
			{
				for (cgltf_size i = 0; i < primVertexCount; ++i)
				{
					out.colors.push_back(tmpColor[i * colorComps + 0]);
					out.colors.push_back(tmpColor[i * colorComps + 1]);
					out.colors.push_back(tmpColor[i * colorComps + 2]);
					out.colors.push_back(colorComps >= 4 ? tmpColor[i * colorComps + 3] : 1.0f);
				}
			}
			else
			{
				for (cgltf_size i = 0; i < primVertexCount; ++i)
				{
					out.colors.push_back(1.0f); out.colors.push_back(1.0f);
					out.colors.push_back(1.0f); out.colors.push_back(1.0f);
				}
			}

			// Skin attributes: append (or pad with zeros so the merged stride
			// stays uniform even when some primitives are skinned and others
			// aren't — rare but legal authoring choice).
			if (accJoints != nullptr && accWeights != nullptr)
			{
				anyHasJoints = true;
				const cgltf_size base = out.jointIndices.size();
				out.jointIndices.resize(base + primVertexCount * 4);
				for (cgltf_size i = 0; i < primVertexCount; ++i)
				{
					cgltf_uint tmp[4] = {};
					if (!cgltf_accessor_read_uint(accJoints, i, tmp, 4))
					{
						cgltf_free(data);
						return SetError(errorOut, prefix + "primitive[" + std::to_string(pi) + "] JOINTS_0 read failed at vertex " + std::to_string(i));
					}
					for (int k = 0; k < 4; ++k)
					{
						out.jointIndices[base + i * 4 + k] = static_cast<std::uint16_t>(tmp[k]);
					}
				}
				if (!UnpackFloatAttribute(accWeights, tmpWeights, errorOut, prefix, "WEIGHTS_0"))
				{
					cgltf_free(data);
					return false;
				}
				if (tmpWeights.size() / 4 != primVertexCount)
				{
					cgltf_free(data);
					return SetError(errorOut, prefix + "primitive[" + std::to_string(pi) + "] WEIGHTS_0 count mismatch");
				}
				out.jointWeights.insert(out.jointWeights.end(), tmpWeights.begin(), tmpWeights.end());
			}
			else if (anyHasJoints)
			{
				// Skinned primitive followed by non-skinned: pad with zeros
				// so the merged stride survives.
				out.jointIndices.resize(out.jointIndices.size() + primVertexCount * 4, 0);
				out.jointWeights.resize(out.jointWeights.size() + primVertexCount * 4, 0.0f);
			}

			// Indices with vertex-base offset. Record the run start so this
			// primitive becomes one SubMesh.
			const std::uint32_t subIndexOffset = static_cast<std::uint32_t>(out.indices.size());
			if (prim.indices != nullptr)
			{
				std::vector<std::uint16_t> tmpInd(prim.indices->count);
				const cgltf_size unpacked = cgltf_accessor_unpack_indices(prim.indices, tmpInd.data(), sizeof(std::uint16_t), prim.indices->count);
				if (unpacked != prim.indices->count)
				{
					cgltf_free(data);
					return SetError(errorOut, prefix + "primitive[" + std::to_string(pi) + "] index unpack failed");
				}
				const std::uint16_t off = static_cast<std::uint16_t>(vertexBase);
				for (auto idx : tmpInd)
				{
					out.indices.push_back(static_cast<std::uint16_t>(idx + off));
				}
			}
			else
			{
				for (cgltf_size i = 0; i < primVertexCount; ++i)
				{
					out.indices.push_back(static_cast<std::uint16_t>(vertexBase + i));
				}
			}
			const std::uint32_t subIndexCount = static_cast<std::uint32_t>(out.indices.size()) - subIndexOffset;

			// Resolve this primitive's baseColorTexture and fold it into the
			// de-duped texture list, then emit a SubMesh tying this index run
			// to that texture (-1 = untextured). De-dup keeps a building's
			// repeated brick faces from re-decoding the same PNG per
			// primitive.
			int texIndex = -1;
			if (prim.material != nullptr && prim.material->has_pbr_metallic_roughness)
			{
				const cgltf_texture_view& bcView = prim.material->pbr_metallic_roughness.base_color_texture;
				if (bcView.texture != nullptr && bcView.texture->image != nullptr)
				{
					const std::string tag = pathStr +
						(bcView.texture->image->name ? std::string("#") + bcView.texture->image->name
						                              : std::string("#image") + std::to_string(pi));
					// Already loaded?
					for (std::size_t ti = 0; ti < out.textures.size(); ++ti)
					{
						if (out.textures[ti].sourceTag == tag) { texIndex = static_cast<int>(ti); break; }
					}
					if (texIndex < 0)
					{
						std::vector<std::uint8_t> bytes;
						if (ResolveImageBytes(*bcView.texture->image, gltfPath, bytes))
						{
							ImportedTexture t;
							t.bytes = std::move(bytes);
							t.sourceTag = tag;
							out.textures.push_back(std::move(t));
							texIndex = static_cast<int>(out.textures.size()) - 1;
						}
					}
				}
			}
			ImportedSubMesh sub;
			sub.indexOffset  = subIndexOffset;
			sub.indexCount   = subIndexCount;
			sub.textureIndex = texIndex; // baseColor (resolved above)
				// PBR maps beyond baseColor. resolveTex folds a texture view into
				// the de-duped texture list, keyed on the image index so each map
				// decodes exactly once.
				auto resolveTex = [&](const cgltf_texture_view& view, bool linear) -> int
				{
					if (view.texture == nullptr || view.texture->image == nullptr) return -1;
					const std::ptrdiff_t imgIdx = view.texture->image - data->images;
					const std::string tag = pathStr + "#img" + std::to_string(imgIdx);
					for (std::size_t ti = 0; ti < out.textures.size(); ++ti)
					{
						if (out.textures[ti].sourceTag == tag) return static_cast<int>(ti);
					}
					std::vector<std::uint8_t> texBytes;
					if (!ResolveImageBytes(*view.texture->image, gltfPath, texBytes)) return -1;
					ImportedTexture t;
					t.bytes = std::move(texBytes);
					t.sourceTag = tag;
					t.isSRGB = !linear; // MR/normal/AO are linear; emissive is sRGB
					out.textures.push_back(std::move(t));
					return static_cast<int>(out.textures.size()) - 1;
				};
				if (prim.material != nullptr)
				{
					const cgltf_material* mat = prim.material;
					if (mat->has_pbr_metallic_roughness)
					{
						const auto& mr = mat->pbr_metallic_roughness;
						sub.metalRoughTexture = resolveTex(mr.metallic_roughness_texture, /*linear=*/true);
						sub.baseColorFactor[0] = mr.base_color_factor[0];
						sub.baseColorFactor[1] = mr.base_color_factor[1];
						sub.baseColorFactor[2] = mr.base_color_factor[2];
						sub.baseColorFactor[3] = mr.base_color_factor[3];
						sub.metallicFactor  = mr.metallic_factor;
						sub.roughnessFactor = mr.roughness_factor;
					}
					sub.normalTexture    = resolveTex(mat->normal_texture,    /*linear=*/true);
					sub.emissiveTexture  = resolveTex(mat->emissive_texture,  /*linear=*/false);
					sub.occlusionTexture = resolveTex(mat->occlusion_texture, /*linear=*/true);
					sub.emissiveFactor[0] = mat->emissive_factor[0];
					sub.emissiveFactor[1] = mat->emissive_factor[1];
					sub.emissiveFactor[2] = mat->emissive_factor[2];
				}
				// Our VColor assets (Quaternius nature) bake the albedo into
				// COLOR_0 while the source glTF *also* keeps a matching
				// baseColorFactor. A correct PBR shader multiplies both, which
				// double-darkens (factor*vcolor) to near-black. When the
				// primitive carries vertex colors, COLOR_0 owns the albedo, so
				// neutralize the redundant factor.
				if (accColor != nullptr)
				{
					sub.baseColorFactor[0] = 1.0f;
					sub.baseColorFactor[1] = 1.0f;
					sub.baseColorFactor[2] = 1.0f;
					sub.baseColorFactor[3] = 1.0f;
				}
			out.submeshes.push_back(sub);
		}

		const cgltf_size vertexCount = out.positions.size() / 3;
		if (vertexCount == 0)
		{
			cgltf_free(data);
			return SetError(errorOut, prefix + "merged mesh has zero vertices");
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

		// Decompose a node's local matrix into TRS. Used as the fallback when
		// a joint node carries `matrix` instead of explicit T/R/S — the
		// animation pipeline always operates in TRS so we normalize at
		// import time.
		auto extractNodeBindTRS = [](const cgltf_node* n, float outT[3], float outR[4], float outS[3])
		{
			using namespace DirectX;
			if (n->has_matrix)
			{
				XMFLOAT4X4 colMajor;
				std::memcpy(&colMajor, n->matrix, sizeof(XMFLOAT4X4));
				const XMMATRIX m = XMMatrixTranspose(XMLoadFloat4x4(&colMajor)); // row-major in DXMath
				XMVECTOR vS, vR, vT;
				if (XMMatrixDecompose(&vS, &vR, &vT, m))
				{
					XMFLOAT3 fS, fT;
					XMFLOAT4 fR;
					XMStoreFloat3(&fS, vS);
					XMStoreFloat4(&fR, vR);
					XMStoreFloat3(&fT, vT);
					outS[0] = fS.x; outS[1] = fS.y; outS[2] = fS.z;
					outR[0] = fR.x; outR[1] = fR.y; outR[2] = fR.z; outR[3] = fR.w;
					outT[0] = fT.x; outT[1] = fT.y; outT[2] = fT.z;
					return;
				}
				// Decompose failure: identity TRS.
				outT[0]=outT[1]=outT[2]=0.0f;
				outR[0]=outR[1]=outR[2]=0.0f; outR[3]=1.0f;
				outS[0]=outS[1]=outS[2]=1.0f;
				return;
			}
			outT[0] = n->has_translation ? n->translation[0] : 0.0f;
			outT[1] = n->has_translation ? n->translation[1] : 0.0f;
			outT[2] = n->has_translation ? n->translation[2] : 0.0f;
			outR[0] = n->has_rotation ? n->rotation[0] : 0.0f;
			outR[1] = n->has_rotation ? n->rotation[1] : 0.0f;
			outR[2] = n->has_rotation ? n->rotation[2] : 0.0f;
			outR[3] = n->has_rotation ? n->rotation[3] : 1.0f;
			outS[0] = n->has_scale ? n->scale[0] : 1.0f;
			outS[1] = n->has_scale ? n->scale[1] : 1.0f;
			outS[2] = n->has_scale ? n->scale[2] : 1.0f;
		};

		if (anyHasJoints && skin && skin->joints_count > 0)
		{
			// JOINTS_0 / WEIGHTS_0 streams were already merged in the
			// primitive loop above; here we only validate the merged
			// counts and continue with skin-level data (IBMs + hierarchy).
			if (out.jointWeights.size() != out.jointIndices.size())
			{
				cgltf_free(data);
				return SetError(errorOut, prefix + "joint/weight stride mismatch after merge");
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
				cgltf_free(data);
				return SetError(errorOut, prefix + "skin.inverseBindMatrices is required (Stage 2 does not synthesize identity defaults)");
			}

			// Stage 3 additions: joint hierarchy + bind-pose local TRS per
			// joint. Both feed the per-frame pose computation in Renderer.
			const cgltf_size jointCount = skin->joints_count;
			out.jointParents.assign(jointCount, -1);
			out.jointBindTranslation.resize(jointCount * 3);
			out.jointBindRotation.resize(jointCount * 4);
			out.jointBindScale.resize(jointCount * 3);
			out.jointParentBaseWorld.assign(jointCount * 16, 0.0f);
			// Initialize each parent-base to identity (col-major); we'll
			// overwrite for root joints with their non-joint parent's
			// world transform below.
			for (cgltf_size i = 0; i < jointCount; ++i)
			{
				float* m = &out.jointParentBaseWorld[i * 16];
				m[0]  = 1.0f; m[5]  = 1.0f; m[10] = 1.0f; m[15] = 1.0f;
			}

			for (cgltf_size i = 0; i < jointCount; ++i)
			{
				const cgltf_node* j = skin->joints[i];
				// Parent index: linear scan over the joint array. Cheap for
				// any realistic rig (RiggedSimple = 2, full character ~ 30).
				bool parentIsJoint = false;
				if (j->parent != nullptr)
				{
					for (cgltf_size k = 0; k < jointCount; ++k)
					{
						if (skin->joints[k] == j->parent)
						{
							out.jointParents[i] = static_cast<int>(k);
							parentIsJoint = true;
							break;
						}
					}
				}
				// Root-of-skin joint: bake the chain of non-joint ancestors
				// (Armature / Z_UP / scene root) into per-joint matrix so
				// the runtime hierarchy walk doesn't have to revisit them.
				if (!parentIsJoint && j->parent != nullptr)
				{
					cgltf_node_transform_world(j->parent, &out.jointParentBaseWorld[i * 16]);
				}
				extractNodeBindTRS(j,
					&out.jointBindTranslation[i * 3],
					&out.jointBindRotation[i * 4],
					&out.jointBindScale[i * 3]);
			}

			// All animation clips in the file (Stage 4 — multi-clip).
			out.animations.resize(data->animations_count);
			for (cgltf_size aIdx = 0; aIdx < data->animations_count; ++aIdx)
			{
				const cgltf_animation& anim = data->animations[aIdx];
				ImportedAnimation& outAnim = out.animations[aIdx];
				outAnim.name = anim.name ? anim.name : "";
				outAnim.duration = 0.0f;

				for (cgltf_size c = 0; c < anim.channels_count; ++c)
				{
					const cgltf_animation_channel& ch = anim.channels[c];
					if (ch.target_node == nullptr || ch.sampler == nullptr) continue;

					// Resolve target_node to a joint index. Channels targeting
					// non-joint nodes (e.g., camera animation) are silently
					// ignored — they are not part of the skin pose.
					int targetJoint = -1;
					for (cgltf_size k = 0; k < jointCount; ++k)
					{
						if (skin->joints[k] == ch.target_node)
						{
							targetJoint = static_cast<int>(k);
							break;
						}
					}
					if (targetJoint < 0) continue;

					ImportedAnimationChannel iac;
					iac.targetJoint = targetJoint;
					switch (ch.target_path)
					{
					case cgltf_animation_path_type_translation: iac.path = ImportedAnimationChannel::Path::Translation; break;
					case cgltf_animation_path_type_rotation:    iac.path = ImportedAnimationChannel::Path::Rotation;    break;
					case cgltf_animation_path_type_scale:       iac.path = ImportedAnimationChannel::Path::Scale;       break;
					default: continue; // weights / unknown — ignored at v1
					}

					switch (ch.sampler->interpolation)
					{
					case cgltf_interpolation_type_linear: iac.interpolation = ImportedAnimationChannel::Interpolation::Linear; break;
					case cgltf_interpolation_type_step:   iac.interpolation = ImportedAnimationChannel::Interpolation::Step;   break;
					case cgltf_interpolation_type_cubic_spline:
						cgltf_free(data);
						return SetError(errorOut, prefix + "CUBICSPLINE interpolation not supported in Stage 3 v1 (re-export with linear)");
					default:
						cgltf_free(data);
						return SetError(errorOut, prefix + "unknown animation interpolation type");
					}

					// Times: SCALAR float accessor.
					const cgltf_accessor* tAcc = ch.sampler->input;
					const cgltf_accessor* vAcc = ch.sampler->output;
					if (tAcc == nullptr || vAcc == nullptr) continue;

					iac.times.resize(tAcc->count);
					if (cgltf_accessor_unpack_floats(tAcc, iac.times.data(), tAcc->count) != tAcc->count)
					{
						cgltf_free(data);
						return SetError(errorOut, prefix + "animation sampler.input unpack failed");
					}

					const cgltf_size valuesPerKey = (iac.path == ImportedAnimationChannel::Path::Rotation) ? 4 : 3;
					iac.values.resize(vAcc->count * valuesPerKey);
					if (cgltf_accessor_unpack_floats(vAcc, iac.values.data(), iac.values.size()) != iac.values.size())
					{
						cgltf_free(data);
						return SetError(errorOut, prefix + "animation sampler.output unpack failed");
					}

					if (!iac.times.empty())
					{
						outAnim.duration = (iac.times.back() > outAnim.duration) ? iac.times.back() : outAnim.duration;
					}
					outAnim.channels.push_back(std::move(iac));
				}
			}
		}

		// ----------------------------------------------------------
		// Static-mesh node transform bake. glTF puts a node's placement
		// (including the Z-up -> Y-up rotation that exporters emit on the
		// root node) on the node, not in the vertex data. Skinned meshes
		// already fold their node hierarchy into joint matrices, but a
		// plain static mesh would otherwise ignore the node transform and
		// render in raw local space — which left exported trees lying on
		// their side. So for the static case we resolve the mesh's node
		// world matrix and bake it into positions (as points) and normals
		// (as directions) here, before the handedness mirror below.
		const bool isSkinned = (anyHasJoints && skin && skin->joints_count > 0);
		if (!isSkinned)
		{
			using namespace DirectX;
			const cgltf_node* meshNode = nullptr;
			for (cgltf_size i = 0; i < data->nodes_count; ++i)
			{
				if (data->nodes[i].mesh == &mesh) { meshNode = &data->nodes[i]; break; }
			}
			if (meshNode != nullptr)
			{
				float colMajor[16];
				cgltf_node_transform_world(meshNode, colMajor);
				// cgltf gives column-major data. Loading those 16 floats
				// straight into a row-major XMFLOAT4X4 already yields the
				// transpose of the glTF matrix, which is exactly the form
				// that left-multiplies a row vector (pos * M == M_gltf * pos).
				// An extra XMMatrixTranspose here would invert the rotation
				// (X+90 -> X-90) and flip the model upside-down.
				XMFLOAT4X4 cm;
				std::memcpy(&cm, colMajor, sizeof(cm));
				const XMMATRIX M = XMLoadFloat4x4(&cm);

				const cgltf_size vcount = out.positions.size() / 3;
				for (cgltf_size i = 0; i < vcount; ++i)
				{
					XMVECTOR p = XMVectorSet(out.positions[i*3+0], out.positions[i*3+1], out.positions[i*3+2], 1.0f);
					p = XMVector3TransformCoord(p, M); // point: applies translation
					XMFLOAT3 fp; XMStoreFloat3(&fp, p);
					out.positions[i*3+0] = fp.x; out.positions[i*3+1] = fp.y; out.positions[i*3+2] = fp.z;
				}
				const cgltf_size ncount = out.normals.size() / 3;
				for (cgltf_size i = 0; i < ncount; ++i)
				{
					XMVECTOR n = XMVectorSet(out.normals[i*3+0], out.normals[i*3+1], out.normals[i*3+2], 0.0f);
					n = XMVector3Normalize(XMVector3TransformNormal(n, M)); // direction: no translation
					XMFLOAT3 fn; XMStoreFloat3(&fn, n);
					out.normals[i*3+0] = fn.x; out.normals[i*3+1] = fn.y; out.normals[i*3+2] = fn.z;
				}

				// Ground the asset: after the node rotation the mesh's
				// lowest point usually isn't at y=0 (the exporter's origin
				// was placed in the source's own space). Shift the whole
				// mesh up so its feet sit on the ground plane, which is what
				// scatter / scene placement assumes for static props. This
				// only runs for static External meshes (trees, rocks,
				// foliage); skinned characters keep their authored pivot.
				if (vcount > 0)
				{
					float minY = out.positions[1];
					for (cgltf_size i = 1; i < vcount; ++i)
					{
						minY = (out.positions[i*3+1] < minY) ? out.positions[i*3+1] : minY;
					}
					for (cgltf_size i = 0; i < vcount; ++i)
					{
						out.positions[i*3+1] -= minY;
					}
				}
			}
		}

		// ----------------------------------------------------------
		// glTF (right-handed) -> our engine (left-handed) conversion.
		//
		// We use the standard X-axis mirror (DirectX / Unity / Unreal
		// convention): flip the X component of every position, normal,
		// translation, and quaternion-X-related rotation, mirror the
		// matrices accordingly, and reverse triangle winding so backface
		// culling stays correct.
		//
		// Without this, a glTF authored as "RightHand bone on the
		// character's own right side" landed on the LEFT side of the
		// rendered character — which made the Mixamo Sword+Shield asset
		// look mirrored in-game. Symmetric meshes (CesiumMan, Fox) are
		// effectively unchanged by the mirror so older assets continue
		// to look the same.
		auto mirrorMatX = [](float* m) noexcept
		{
			// Column-major 4x4: flip row 0 (entries with j!=0) and col 0
			// (entries with i!=0). Algebraically: S @ M @ S where
			// S = diag(-1, 1, 1, 1).
			m[1]  = -m[1];   // col 0 row 1
			m[2]  = -m[2];   // col 0 row 2
			m[3]  = -m[3];   // col 0 row 3
			m[4]  = -m[4];   // col 1 row 0
			m[8]  = -m[8];   // col 2 row 0
			m[12] = -m[12];  // col 3 row 0 (translation X)
		};

		for (cgltf_size i = 0; i < out.positions.size(); i += 3) out.positions[i] = -out.positions[i];
		for (cgltf_size i = 0; i < out.normals.size();   i += 3) out.normals[i]   = -out.normals[i];

		// Triangle winding: glTF authors CCW front faces; after the X
		// mirror those become CW, and our rasterizer culls CW backfaces
		// by default — without this swap the whole model culls inside-out.
		for (cgltf_size i = 0; i + 2 < out.indices.size(); i += 3)
		{
			std::swap(out.indices[i + 1], out.indices[i + 2]);
		}

		// Skin bind data + cached parent-base matrices need the same mirror.
		for (cgltf_size i = 0; i < out.jointBindTranslation.size(); i += 3)
		{
			out.jointBindTranslation[i] = -out.jointBindTranslation[i];
		}
		for (cgltf_size i = 0; i + 3 < out.jointBindRotation.size(); i += 4)
		{
			// Quaternion mirror across YZ plane: (qx, qy, qz, qw) becomes
			// (qx, -qy, -qz, qw). Derivation: mirrored rotation axis is
			// (-ax, ay, az), mirrored angle is -θ, and the two minus signs
			// in q = sin(θ/2)*axis collapse onto qy/qz only.
			out.jointBindRotation[i + 1] = -out.jointBindRotation[i + 1];
			out.jointBindRotation[i + 2] = -out.jointBindRotation[i + 2];
		}
		for (cgltf_size i = 0; i + 15 < out.inverseBindMatrices.size(); i += 16)
		{
			mirrorMatX(&out.inverseBindMatrices[i]);
		}
		for (cgltf_size i = 0; i + 15 < out.jointParentBaseWorld.size(); i += 16)
		{
			mirrorMatX(&out.jointParentBaseWorld[i]);
		}

		// Animation keyframes: translation X flips, rotation qy/qz flip,
		// scale is preserved.
		for (auto& anim : out.animations)
		{
			for (auto& ch : anim.channels)
			{
				if (ch.path == ImportedAnimationChannel::Path::Translation)
				{
					for (cgltf_size i = 0; i < ch.values.size(); i += 3) ch.values[i] = -ch.values[i];
				}
				else if (ch.path == ImportedAnimationChannel::Path::Rotation)
				{
					for (cgltf_size i = 0; i + 3 < ch.values.size(); i += 4)
					{
						ch.values[i + 1] = -ch.values[i + 1];
						ch.values[i + 2] = -ch.values[i + 2];
					}
				}
			}
		}

		cgltf_free(data);
		return true;
	}
}
