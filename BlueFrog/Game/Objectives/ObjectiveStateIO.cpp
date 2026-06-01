#include "ObjectiveStateIO.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace
{
	bool SetError(std::string* sink, std::string msg)
	{
		if (sink != nullptr)
		{
			*sink = std::move(msg);
		}
		return false;
	}

	// Narrow UTF-8 (std::string) to wchar_t for title-bar rendering. Scene
	// JSON holds ASCII-only strings in practice, so we widen 1:1. A
	// non-ASCII byte would simply appear as its code-point value; objectives
	// never carry one. (Was WidenAscii in SceneLoader; relocated together
	// with the parser to keep the conversion next to its consumer.)
	std::wstring WidenAscii(const std::string& s)
	{
		return std::wstring(s.begin(), s.end());
	}

	// Reverse of WidenAscii. Non-ASCII codepoints drop to '?' rather than
	// failing the save — objectives are validator-checked to be ASCII at
	// parse time so this should never fire in practice.
	std::string NarrowAscii(const std::wstring& w)
	{
		std::string out;
		out.reserve(w.size());
		for (wchar_t c : w)
		{
			out.push_back((c >= 0x20 && c < 0x80) ? static_cast<char>(c) : '?');
		}
		return out;
	}

	// Parses a single leaf-shaped JSON object into an ObjectiveLeaf. Used
	// both for top-level leaf conditions and for the entries inside an
	// "any" group's "anyOf" array. Rejects unknown types and non-positive
	// counts. `pathPrefix` is included in every error message verbatim.
	bool ParseLeaf(const json& leafNode, const std::string& pathPrefix, ObjectiveLeaf& out, std::string* errorOut)
	{
		if (!leafNode.is_object())
		{
			return SetError(errorOut, pathPrefix + "objective leaf must be a JSON object");
		}
		out.type = leafNode.value("type", std::string{});
		out.name = leafNode.value("name", std::string{});

		// Leaf allow-list. enemy_killed is event-driven (ObjectiveSystem /
		// QuestSystem::Consume); collect_item is inventory-driven (quest only,
		// synced by QuestSystem::SyncCollectProgress — name = item id).
		if (out.type != "enemy_killed" && out.type != "collect_item")
		{
			return SetError(errorOut, pathPrefix + "objective leaf: unknown type '" + out.type + "' (expected 'enemy_killed' or 'collect_item')");
		}

		// "count" is optional; absence means 1 (the v1 single-kill default).
		out.required = leafNode.value("count", 1);
		if (out.required < 1)
		{
			return SetError(errorOut, pathPrefix + "objective leaf 'count' must be >= 1 (got " + std::to_string(out.required) + ")");
		}
		out.progress = 0;
		return true;
	}

	json EncodeLeaf(const ObjectiveLeaf& leaf)
	{
		json j = json::object();
		j["type"] = leaf.type;
		j["name"] = leaf.name;
		// Only emit `count` when it differs from the v1 default; a count
		// of 1 is implicit and writing it back would noisily diff against
		// the hand-authored scenes that omit the field.
		if (leaf.required != 1)
		{
			j["count"] = leaf.required;
		}
		// `progress` is runtime state, not part of the schema.
		return j;
	}
}

namespace ObjectiveStateIO
{
	bool ParseJson(const std::string& objectiveBlockJsonText,
		const std::string& pathPrefix,
		ObjectiveState& out,
		std::string* errorOut)
	{
		out = {};
		if (objectiveBlockJsonText.empty())
		{
			// Absent block ↔ empty state. Loader contracts on this.
			return true;
		}

		json objNode;
		try
		{
			objNode = json::parse(objectiveBlockJsonText);
		}
		catch (const json::parse_error& e)
		{
			return SetError(errorOut, pathPrefix + "objective JSON parse error: " + e.what());
		}
		if (!objNode.is_object())
		{
			return SetError(errorOut, pathPrefix + "objective must be a JSON object");
		}

		out.text           = WidenAscii(objNode.value("text", std::string{}));
		out.completionText = WidenAscii(objNode.value("completionText", std::string{}));
		out.conditions.clear();

		if (!objNode.contains("conditions"))
		{
			return true;
		}
		if (!objNode["conditions"].is_array())
		{
			return SetError(errorOut, pathPrefix + "objective.conditions must be an array");
		}

		for (const auto& c : objNode["conditions"])
		{
			if (!c.is_object())
			{
				return SetError(errorOut, pathPrefix + "objective.conditions entry must be a JSON object");
			}

			ObjectiveCondition cond;
			const std::string slotType = c.value("type", std::string{});

			if (slotType == "any")
			{
				// OR group. The slot itself carries no name/count; "anyOf"
				// lists the leaves whose disjunction is the slot's truth
				// value.
				if (!c.contains("anyOf") || !c["anyOf"].is_array() || c["anyOf"].empty())
				{
					return SetError(errorOut, pathPrefix + "objective 'any' condition requires non-empty 'anyOf' array");
				}
				for (const auto& leafNode : c["anyOf"])
				{
					ObjectiveLeaf leaf;
					if (!ParseLeaf(leafNode, pathPrefix, leaf, errorOut))
					{
						return false;
					}
					cond.leaves.push_back(std::move(leaf));
				}
			}
			else
			{
				// Single-leaf condition (v1 shape). The leaf is the slot
				// itself.
				ObjectiveLeaf leaf;
				if (!ParseLeaf(c, pathPrefix, leaf, errorOut))
				{
					return false;
				}
				cond.leaves.push_back(std::move(leaf));
			}

			out.conditions.push_back(std::move(cond));
		}
		return true;
	}

	std::string EncodeJson(const ObjectiveState& state)
	{
		// Empty state → empty string so the caller can omit the "objective"
		// key entirely. Loader treats absence and empty-object identically.
		if (state.text.empty() && state.completionText.empty() && state.conditions.empty())
		{
			return {};
		}

		json j = json::object();
		j["text"]           = NarrowAscii(state.text);
		j["completionText"] = NarrowAscii(state.completionText);

		json conds = json::array();
		for (const auto& cond : state.conditions)
		{
			if (cond.leaves.size() == 1)
			{
				conds.push_back(EncodeLeaf(cond.leaves[0]));
			}
			else
			{
				json group = json::object();
				group["type"]  = "any";
				json anyOf = json::array();
				for (const auto& leaf : cond.leaves)
				{
					anyOf.push_back(EncodeLeaf(leaf));
				}
				group["anyOf"] = std::move(anyOf);
				conds.push_back(std::move(group));
			}
		}
		j["conditions"] = std::move(conds);
		return j.dump();
	}
}
