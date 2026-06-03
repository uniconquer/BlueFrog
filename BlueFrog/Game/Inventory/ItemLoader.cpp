#include "ItemLoader.h"

#include <nlohmann/json.hpp>

#include <fstream>

namespace
{
	bool SetErr(std::string* out, std::string msg)
	{
		if (out) *out = std::move(msg);
		return false;
	}

	std::string PathPrefix(const std::filesystem::path& path)
	{
		return path.string() + ": ";
	}

	std::wstring Widen(const std::string& s)
	{
		std::wstring out;
		out.reserve(s.size());
		for (char c : s)
		{
			out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
		}
		return out;
	}
}

namespace ItemLoader
{
	bool Load(const std::filesystem::path& path, Item& out, std::string* errorOut)
	{
		std::ifstream file(path);
		if (!file.is_open())
		{
			return SetErr(errorOut, PathPrefix(path) + "cannot open item file");
		}

		nlohmann::json j;
		try
		{
			j = nlohmann::json::parse(file);
		}
		catch (const nlohmann::json::parse_error& e)
		{
			return SetErr(errorOut, PathPrefix(path) + "JSON parse error: " + e.what());
		}
		if (!j.is_object())
		{
			return SetErr(errorOut, PathPrefix(path) + "item root must be a JSON object");
		}

		out = Item{};
		if (!j.contains("id") || !j["id"].is_string())
		{
			return SetErr(errorOut, PathPrefix(path) + "item missing required 'id' string");
		}
		out.id = j["id"].get<std::string>();
		if (out.id.empty())
		{
			return SetErr(errorOut, PathPrefix(path) + "item 'id' must be non-empty");
		}

		if (j.contains("name"))        out.name        = Widen(j["name"].get<std::string>());
		if (j.contains("description")) out.description = Widen(j["description"].get<std::string>());
		if (j.contains("icon"))        out.icon        = Widen(j["icon"].get<std::string>());
		if (j.contains("maxStack"))    out.maxStack    = j["maxStack"].get<int>();
		if (out.maxStack < 1) out.maxStack = 1; // never let a typo collapse the stack

		if (j.contains("effect") && j["effect"].is_object())
		{
			const auto& e = j["effect"];
			if (e.contains("heal"))           out.effect.heal           = e["heal"].get<int>();
			if (e.contains("boostMaxHealth")) out.effect.boostMaxHealth = e["boostMaxHealth"].get<int>();
		}

		return true;
	}
}
