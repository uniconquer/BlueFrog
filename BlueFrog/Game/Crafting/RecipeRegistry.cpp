#include "RecipeRegistry.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

using json = nlohmann::json;

namespace
{
	std::wstring Widen(const std::string& s)
	{
		std::wstring out;
		out.reserve(s.size());
		for (char c : s) out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
		return out;
	}
}

bool RecipeRegistry::LoadAll(const std::filesystem::path& directory, std::string* errorOut)
{
	recipes_.clear();
	std::error_code ec;
	if (!std::filesystem::exists(directory, ec))
	{
		return true; // missing directory = no recipes, not an error
	}

	for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
	{
		if (!entry.is_regular_file()) continue;
		const auto& p = entry.path();
		// Match "*.recipe.json".
		const std::string name = p.filename().string();
		if (name.size() < 12 || name.substr(name.size() - 12) != ".recipe.json") continue;

		std::ifstream f(p);
		if (!f)
		{
			if (errorOut) *errorOut = "recipe open failed: " + p.string();
			return false;
		}
		json j;
		try { j = json::parse(f); }
		catch (const json::parse_error& e)
		{
			if (errorOut) *errorOut = "recipe parse error in " + p.string() + ": " + e.what();
			return false;
		}

		Recipe r;
		r.id           = j.value("id", std::string());
		r.name         = Widen(j.value("name", r.id));
		r.outputItemId = j.value("output", std::string());
		r.outputCount  = j.value("outputCount", 1);
		if (j.contains("inputs") && j["inputs"].is_array())
		{
			for (const auto& in : j["inputs"])
			{
				RecipeInput ri;
				ri.itemId = in.value("itemId", std::string());
				ri.count  = in.value("count", 1);
				if (!ri.itemId.empty()) r.inputs.push_back(std::move(ri));
			}
		}
		if (r.id.empty() || r.outputItemId.empty())
		{
			if (errorOut) *errorOut = "recipe missing id/output in " + p.string();
			return false;
		}
		recipes_.push_back(std::move(r));
	}

	// Stable slot order for the crafting UI.
	std::sort(recipes_.begin(), recipes_.end(),
		[](const Recipe& a, const Recipe& b) { return a.id < b.id; });
	return true;
}
