#include "MeshImporter.h"

#include <nlohmann/json.hpp>

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
	using json = nlohmann::json;

	std::string PathPrefix(const std::filesystem::path& p)
	{
		return p.string() + ": ";
	}

	bool SetError(std::string* out, std::string msg)
	{
		if (out) *out = std::move(msg);
		return false;
	}

	// glTF componentType constants we care about.
	constexpr int CT_FLOAT          = 5126;
	constexpr int CT_UNSIGNED_SHORT = 5123;
	constexpr int CT_UNSIGNED_INT   = 5125;

	// glTF primitive mode 4 = TRIANGLES.
	constexpr int MODE_TRIANGLES = 4;

	// Base64 decoder. Standard alphabet, no URL-safe variant. Whitespace and
	// padding handled minimally — glTF data: URIs do not pad mid-stream.
	bool DecodeBase64(const std::string& in, std::vector<std::uint8_t>& out)
	{
		std::int8_t lut[256];
		for (int i = 0; i < 256; ++i) lut[i] = -1;
		for (int i = 0; i < 26; ++i) lut['A' + i] = static_cast<std::int8_t>(i);
		for (int i = 0; i < 26; ++i) lut['a' + i] = static_cast<std::int8_t>(26 + i);
		for (int i = 0; i < 10; ++i) lut['0' + i] = static_cast<std::int8_t>(52 + i);
		lut[static_cast<unsigned>('+')] = 62;
		lut[static_cast<unsigned>('/')] = 63;

		out.clear();
		out.reserve((in.size() * 3) / 4);

		int buf = 0;
		int bits = 0;
		for (char c : in)
		{
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

	// Resolve a "data:..;base64,...." URI into raw bytes. External .bin file
	// references are not supported in v1.
	bool ResolveBufferUri(const std::string& uri, std::vector<std::uint8_t>& out, std::string* errorOut, const std::string& prefix)
	{
		const std::string base64Marker = ";base64,";
		const auto markerPos = uri.find(base64Marker);
		if (uri.rfind("data:", 0) != 0 || markerPos == std::string::npos)
		{
			return SetError(errorOut, prefix + "buffer.uri must be a 'data:...;base64,' URI (external .bin is not supported in v1)");
		}
		const std::string b64 = uri.substr(markerPos + base64Marker.size());
		if (!DecodeBase64(b64, out))
		{
			return SetError(errorOut, prefix + "buffer.uri base64 decode failed");
		}
		return true;
	}

	struct AccessorInfo
	{
		int  componentType = 0;
		int  count         = 0;
		int  components    = 0; // 1 SCALAR / 2 VEC2 / 3 VEC3 / 4 VEC4
		int  bufferView    = 0;
		int  byteOffset    = 0;
	};

	bool TypeToComponents(const std::string& t, int& out)
	{
		if (t == "SCALAR") { out = 1; return true; }
		if (t == "VEC2")   { out = 2; return true; }
		if (t == "VEC3")   { out = 3; return true; }
		if (t == "VEC4")   { out = 4; return true; }
		return false;
	}

	bool ReadAccessor(const json& accessorsArr, int idx, AccessorInfo& out, std::string* errorOut, const std::string& prefix)
	{
		if (idx < 0 || idx >= static_cast<int>(accessorsArr.size()))
		{
			return SetError(errorOut, prefix + "accessor index out of range: " + std::to_string(idx));
		}
		const auto& a = accessorsArr[idx];
		out.componentType = a.value("componentType", 0);
		out.count         = a.value("count", 0);
		out.bufferView    = a.value("bufferView", 0);
		out.byteOffset    = a.value("byteOffset", 0);
		const std::string typeStr = a.value("type", std::string{});
		if (!TypeToComponents(typeStr, out.components))
		{
			return SetError(errorOut, prefix + "accessor[" + std::to_string(idx) + "].type unsupported: '" + typeStr + "'");
		}
		return true;
	}

	struct BufferViewInfo
	{
		int buffer     = 0;
		int byteOffset = 0;
		int byteLength = 0;
	};

	bool ReadBufferView(const json& bufferViewsArr, int idx, BufferViewInfo& out, std::string* errorOut, const std::string& prefix)
	{
		if (idx < 0 || idx >= static_cast<int>(bufferViewsArr.size()))
		{
			return SetError(errorOut, prefix + "bufferView index out of range: " + std::to_string(idx));
		}
		const auto& bv = bufferViewsArr[idx];
		out.buffer     = bv.value("buffer", 0);
		out.byteOffset = bv.value("byteOffset", 0);
		out.byteLength = bv.value("byteLength", 0);
		return true;
	}

	// Copy `count` * `components` floats out of the buffer at the accessor's
	// effective offset. componentType must be FLOAT for the v1 attribute path.
	bool ExtractFloatStream(const std::vector<std::uint8_t>& buffer,
		const AccessorInfo& acc,
		const BufferViewInfo& bv,
		std::vector<float>& out,
		std::string* errorOut,
		const std::string& prefix)
	{
		if (acc.componentType != CT_FLOAT)
		{
			return SetError(errorOut, prefix + "vertex attribute componentType must be FLOAT (5126), got " + std::to_string(acc.componentType));
		}
		const std::size_t totalFloats = static_cast<std::size_t>(acc.count) * static_cast<std::size_t>(acc.components);
		const std::size_t totalBytes  = totalFloats * sizeof(float);
		const std::size_t startByte   = static_cast<std::size_t>(bv.byteOffset) + static_cast<std::size_t>(acc.byteOffset);
		if (startByte + totalBytes > buffer.size())
		{
			return SetError(errorOut, prefix + "accessor range exceeds buffer length");
		}
		out.resize(totalFloats);
		std::memcpy(out.data(), buffer.data() + startByte, totalBytes);
		return true;
	}

	bool ExtractIndexStream(const std::vector<std::uint8_t>& buffer,
		const AccessorInfo& acc,
		const BufferViewInfo& bv,
		std::vector<std::uint16_t>& out,
		std::string* errorOut,
		const std::string& prefix)
	{
		const std::size_t startByte = static_cast<std::size_t>(bv.byteOffset) + static_cast<std::size_t>(acc.byteOffset);
		if (acc.componentType == CT_UNSIGNED_SHORT)
		{
			const std::size_t totalBytes = static_cast<std::size_t>(acc.count) * sizeof(std::uint16_t);
			if (startByte + totalBytes > buffer.size())
			{
				return SetError(errorOut, prefix + "index accessor range exceeds buffer length");
			}
			out.resize(static_cast<std::size_t>(acc.count));
			std::memcpy(out.data(), buffer.data() + startByte, totalBytes);
			return true;
		}
		if (acc.componentType == CT_UNSIGNED_INT)
		{
			return SetError(errorOut, prefix + "32-bit indices not supported in v1 (mesh exceeds 65535 vertices?). Re-export with 16-bit indices.");
		}
		return SetError(errorOut, prefix + "index componentType must be UNSIGNED_SHORT (5123), got " + std::to_string(acc.componentType));
	}
}

namespace MeshImporter
{
	bool Load(const std::filesystem::path& gltfPath, ImportedMesh& out, std::string* errorOut) noexcept
	{
		const std::string prefix = PathPrefix(gltfPath);
		out = {};

		std::ifstream file(gltfPath);
		if (!file.is_open())
		{
			return SetError(errorOut, prefix + "cannot open file");
		}

		json root;
		try
		{
			root = json::parse(file);
		}
		catch (const json::parse_error& e)
		{
			return SetError(errorOut, prefix + "JSON parse error: " + e.what());
		}

		// 1. Asset version sanity. 2.0 is what we author and what cgltf uses.
		if (!root.contains("asset") || !root["asset"].contains("version") ||
			root["asset"]["version"].get<std::string>() != "2.0")
		{
			return SetError(errorOut, prefix + "asset.version must be '2.0'");
		}

		// 2. Required arrays.
		if (!root.contains("meshes") || !root["meshes"].is_array() || root["meshes"].empty())
		{
			return SetError(errorOut, prefix + "missing or empty 'meshes' array");
		}
		if (!root.contains("accessors") || !root["accessors"].is_array())
		{
			return SetError(errorOut, prefix + "missing 'accessors' array");
		}
		if (!root.contains("bufferViews") || !root["bufferViews"].is_array())
		{
			return SetError(errorOut, prefix + "missing 'bufferViews' array");
		}
		if (!root.contains("buffers") || !root["buffers"].is_array() || root["buffers"].empty())
		{
			return SetError(errorOut, prefix + "missing or empty 'buffers' array");
		}

		// 3. Resolve buffer 0 (we only support a single buffer in v1).
		std::vector<std::uint8_t> rawBuffer;
		{
			const auto& buf = root["buffers"][0];
			if (!buf.contains("uri"))
			{
				return SetError(errorOut, prefix + "buffer[0] missing 'uri' (v1 requires data: URI)");
			}
			if (!ResolveBufferUri(buf["uri"].get<std::string>(), rawBuffer, errorOut, prefix))
			{
				return false;
			}
		}

		// 4. First mesh, first primitive. v1 takes only one primitive per mesh.
		const auto& mesh = root["meshes"][0];
		if (!mesh.contains("primitives") || !mesh["primitives"].is_array() || mesh["primitives"].empty())
		{
			return SetError(errorOut, prefix + "meshes[0].primitives missing or empty");
		}
		const auto& prim = mesh["primitives"][0];

		const int mode = prim.value("mode", MODE_TRIANGLES);
		if (mode != MODE_TRIANGLES)
		{
			return SetError(errorOut, prefix + "primitive.mode must be 4 (TRIANGLES), got " + std::to_string(mode));
		}
		if (!prim.contains("attributes") || !prim.contains("indices"))
		{
			return SetError(errorOut, prefix + "primitive must have 'attributes' and 'indices'");
		}

		const auto& attrs       = prim["attributes"];
		const auto& accessorsJ  = root["accessors"];
		const auto& bufferViewsJ = root["bufferViews"];

		auto extractAttribute = [&](const char* attrName, std::vector<float>& dst) -> bool
		{
			if (!attrs.contains(attrName)) return true; // optional
			AccessorInfo acc; BufferViewInfo bv;
			if (!ReadAccessor(accessorsJ, attrs[attrName].get<int>(), acc, errorOut, prefix)) return false;
			if (!ReadBufferView(bufferViewsJ, acc.bufferView, bv, errorOut, prefix)) return false;
			return ExtractFloatStream(rawBuffer, acc, bv, dst, errorOut, prefix);
		};

		// POSITION is required.
		if (!attrs.contains("POSITION"))
		{
			return SetError(errorOut, prefix + "primitive.attributes missing required 'POSITION'");
		}
		if (!extractAttribute("POSITION", out.positions)) return false;
		if (!extractAttribute("NORMAL",   out.normals))   return false;
		if (!extractAttribute("TEXCOORD_0", out.uvs))     return false;

		// Indices.
		{
			AccessorInfo acc; BufferViewInfo bv;
			if (!ReadAccessor(accessorsJ, prim["indices"].get<int>(), acc, errorOut, prefix)) return false;
			if (!ReadBufferView(bufferViewsJ, acc.bufferView, bv, errorOut, prefix)) return false;
			if (acc.components != 1)
			{
				return SetError(errorOut, prefix + "indices accessor.type must be SCALAR");
			}
			if (!ExtractIndexStream(rawBuffer, acc, bv, out.indices, errorOut, prefix)) return false;
		}

		// Sanity: vertex count consistency across streams.
		const std::size_t vertexCount = out.positions.size() / 3;
		if (vertexCount == 0)
		{
			return SetError(errorOut, prefix + "primitive has zero vertices");
		}
		if (!out.normals.empty() && out.normals.size() / 3 != vertexCount)
		{
			return SetError(errorOut, prefix + "NORMAL count does not match POSITION count");
		}
		if (!out.uvs.empty() && out.uvs.size() / 2 != vertexCount)
		{
			return SetError(errorOut, prefix + "TEXCOORD_0 count does not match POSITION count");
		}

		return true;
	}
}
