#include "QuestLoader.h"

#include "../Objectives/ObjectiveStateIO.h"

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

	// Cheap ASCII-narrow → wide. Quest dialog lines are validator-bound
	// to ASCII same as the rest of the scene schema.
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

namespace QuestLoader
{
	bool Load(const std::filesystem::path& path, Quest& out, std::string* errorOut)
	{
		std::ifstream file(path);
		if (!file.is_open())
		{
			return SetErr(errorOut, PathPrefix(path) + "cannot open quest file");
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
			return SetErr(errorOut, PathPrefix(path) + "quest root must be a JSON object");
		}

		out = Quest{};
		if (!j.contains("id") || !j["id"].is_string())
		{
			return SetErr(errorOut, PathPrefix(path) + "quest missing required 'id' string");
		}
		out.id = j["id"].get<std::string>();
		if (out.id.empty())
		{
			return SetErr(errorOut, PathPrefix(path) + "quest 'id' must be non-empty");
		}

		if (j.contains("title"))         out.title         = Widen(j["title"].get<std::string>());
		if (j.contains("dialogOffer"))    out.dialogOffer    = Widen(j["dialogOffer"].get<std::string>());
		if (j.contains("dialogActive"))   out.dialogActive   = Widen(j["dialogActive"].get<std::string>());
		if (j.contains("dialogComplete")) out.dialogComplete = Widen(j["dialogComplete"].get<std::string>());
		if (j.contains("dialogTurnedIn")) out.dialogTurnedIn = Widen(j["dialogTurnedIn"].get<std::string>());

		// Conditions piggy-back on ObjectiveStateIO so the same "any"
		// group / count-N parser handles both scene objectives and
		// quest conditions — no duplicate validation code.
		if (j.contains("conditions"))
		{
			nlohmann::json objBlock = nlohmann::json::object();
			objBlock["conditions"] = j["conditions"];
			ObjectiveState scratch;
			if (!ObjectiveStateIO::ParseJson(objBlock.dump(), PathPrefix(path), scratch, errorOut))
			{
				return false;
			}
			out.conditions = std::move(scratch.conditions);
		}

		if (j.contains("reward") && j["reward"].is_object())
		{
			const auto& r = j["reward"];
			if (r.contains("healPlayer"))     out.reward.healPlayer     = r["healPlayer"].get<int>();
			if (r.contains("boostMaxHealth")) out.reward.boostMaxHealth = r["boostMaxHealth"].get<int>();
			if (r.contains("itemId"))         out.reward.itemId         = r["itemId"].get<std::string>();
			if (r.contains("itemQuantity"))   out.reward.itemQuantity   = r["itemQuantity"].get<int>();
		}

		return true;
	}
}
