#include "SkillLoader.h"

#include <nlohmann/json.hpp>

#include <fstream>

namespace
{
	bool SetErr(std::string* out, std::string msg)
	{
		if (out) *out = std::move(msg);
		return false;
	}

	std::string PathPrefix(const std::filesystem::path& p)
	{
		return p.string() + ": ";
	}
}

namespace SkillLoader
{
	bool Load(const std::filesystem::path& path, Skill& out, std::string* errorOut)
	{
		std::ifstream f(path);
		if (!f.is_open()) return SetErr(errorOut, PathPrefix(path) + "cannot open skill file");

		nlohmann::json j;
		try { j = nlohmann::json::parse(f); }
		catch (const nlohmann::json::parse_error& e)
		{
			return SetErr(errorOut, PathPrefix(path) + "JSON parse error: " + e.what());
		}
		if (!j.is_object()) return SetErr(errorOut, PathPrefix(path) + "skill root must be a JSON object");

		out = Skill{};
		if (!j.contains("id") || !j["id"].is_string())
			return SetErr(errorOut, PathPrefix(path) + "skill missing 'id'");
		out.id = j["id"].get<std::string>();
		if (out.id.empty()) return SetErr(errorOut, PathPrefix(path) + "skill 'id' must be non-empty");

		if (j.contains("name"))          out.name          = j["name"].get<std::string>();
		if (j.contains("animationClip")) out.animationClip = j["animationClip"].get<std::string>();
		if (j.contains("duration"))      out.duration      = j["duration"].get<float>();
		if (j.contains("cooldown"))      out.cooldown      = j["cooldown"].get<float>();

		if (j.contains("events") && j["events"].is_array())
		{
			for (const auto& e : j["events"])
			{
				SkillEvent ev;
				if (e.contains("type"))   ev.type   = e["type"].get<std::string>();
				if (e.contains("time"))   ev.time   = e["time"].get<float>();
				if (e.contains("amount")) ev.amount = e["amount"].get<int>();
				if (e.contains("range"))  ev.range  = e["range"].get<float>();
				// v1 type allow-list — typos surface at boot rather than
				// during a fight.
				if (ev.type != "damage" && ev.type != "particle")
				{
					return SetErr(errorOut, PathPrefix(path) + "skill event unknown type '" + ev.type + "' (expected damage|particle)");
				}
				out.events.push_back(std::move(ev));
			}
		}
		return true;
	}
}
