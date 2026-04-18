#include "SceneLoader.h"
#include "CollisionComponent.h"
#include "CombatComponent.h"
#include "Material.h"
#include "PrefabLoader.h"
#include "RenderComponent.h"
#include "SceneObject.h"
#include "Transform.h"
#include "TriggerComponent.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

// ---- helpers ---------------------------------------------------------------

// Forward declaration: ParseObjective (defined mid-file) emits path-prefixed
// error messages via PathPrefix, which lives near the public interface below.
static std::string PathPrefix(const std::filesystem::path& path);

static bool SetError(std::string* out, std::string msg)
{
	if (out) *out = std::move(msg);
	return false;
}

static RenderMeshType ParseMeshType(const std::string& s)
{
	if (s == "plane") return RenderMeshType::Plane;
	return RenderMeshType::Cube;
}

static SamplerPreset ParseSamplerPreset(const std::string& s)
{
	if (s == "clamp_linear") return SamplerPreset::ClampLinear;
	if (s == "wrap_point")   return SamplerPreset::WrapPoint;
	return SamplerPreset::WrapLinear;
}

static DirectX::XMFLOAT3 ParseFloat3(const json& arr)
{
	return { arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>() };
}

static DirectX::XMFLOAT2 ParseFloat2(const json& arr)
{
	return { arr[0].get<float>(), arr[1].get<float>() };
}

static CombatFaction ParseFaction(const std::string& s)
{
	if (s == "enemy")   return CombatFaction::Enemy;
	if (s == "neutral") return CombatFaction::Neutral;
	return CombatFaction::Player;
}

// ---- component parsers -----------------------------------------------------

static void ParseTransform(const json& j, Transform& t)
{
	if (j.contains("position")) t.position = ParseFloat3(j["position"]);
	if (j.contains("rotation")) t.rotation = ParseFloat3(j["rotation"]);
	if (j.contains("scale"))    t.scale    = ParseFloat3(j["scale"]);
}

static RenderComponent ParseRender(const json& j)
{
	RenderComponent rc;
	if (j.contains("mesh"))
	{
		rc.meshType = ParseMeshType(j["mesh"].get<std::string>());
	}

	if (j.contains("material"))
	{
		const auto& m = j["material"];
		Material mat;
		if (m.contains("texture")) mat.texturePath = m["texture"].get<std::string>();
		if (m.contains("tint"))    mat.tint         = ParseFloat3(m["tint"]);
		if (m.contains("sampler")) mat.sampler       = ParseSamplerPreset(m["sampler"].get<std::string>());
		rc.material = std::move(mat);
	}
	return rc;
}

static CollisionComponent ParseCollision(const json& j)
{
	CollisionComponent cc;
	if (j.contains("halfExtents")) cc.halfExtents = ParseFloat2(j["halfExtents"]);
	if (j.contains("blocking"))    cc.blocksMovement = j["blocking"].get<bool>();
	return cc;
}

static CombatComponent ParseCombat(const json& j)
{
	CombatComponent cc;
	if (j.contains("faction"))   cc.faction   = ParseFaction(j["faction"].get<std::string>());
	if (j.contains("maxHp"))     cc.maxHealth  = j["maxHp"].get<int>();
	if (j.contains("currentHp")) cc.health     = j["currentHp"].get<int>();
	return cc;
}

// v1 trigger-action allow-list. Keep in sync with TriggerGameplaySystem's
// dispatch switch — adding a new action type means a new case there and a
// new entry here (and usually a matching GameEventType).
static bool IsKnownTriggerActionType(const std::string& s)
{
	return s == "log" || s == "publish" || s == "load_scene";
}

// Returns true and leaves `outAction` populated if parsing succeeds. Returns
// false with errorOut filled if the action block is malformed or uses an
// unknown type. Absence of an "action" key is success with outAction unset
// (equivalent to type "log").
static bool ParseTriggerAction(const json& j, const std::filesystem::path& path, std::optional<TriggerAction>& outAction, std::string* errorOut)
{
	if (!j.contains("action"))
	{
		return true;
	}
	const json& a = j["action"];
	if (!a.is_object())
	{
		return SetError(errorOut, PathPrefix(path) + "trigger.action must be a JSON object");
	}
	TriggerAction action;
	action.type  = a.value("type", std::string{});
	action.param = a.value("param", std::string{});
	if (!IsKnownTriggerActionType(action.type))
	{
		return SetError(errorOut, PathPrefix(path) + "trigger.action.type: unknown '" + action.type + "' (expected 'log', 'publish', or 'load_scene')");
	}
	outAction = std::move(action);
	return true;
}

static bool ParseTrigger(const json& j, const std::filesystem::path& path, TriggerComponent& outTc, std::string* errorOut)
{
	if (j.contains("halfExtents")) outTc.halfExtents = ParseFloat2(j["halfExtents"]);
	if (j.contains("tag"))         outTc.tag         = j["tag"].get<std::string>();
	if (j.contains("fireOnce"))    outTc.fireOnce    = j["fireOnce"].get<bool>();
	return ParseTriggerAction(j, path, outTc.action, errorOut);
}

// ---- objective parsing ------------------------------------------------------

// Narrow UTF-8 (std::string) to wchar_t for title-bar rendering. Scene JSON
// holds ASCII-only strings in practice, so we widen 1:1. A non-ASCII byte
// would simply appear as its code-point value; objectives never carry one.
static std::wstring WidenAscii(const std::string& s)
{
	return std::wstring(s.begin(), s.end());
}

static bool ParseObjective(const json& objNode, const std::filesystem::path& path, ObjectiveState& out, std::string* errorOut)
{
	out.text           = WidenAscii(objNode.value("text", std::string{}));
	out.completionText = WidenAscii(objNode.value("completionText", std::string{}));
	out.conditions.clear();

	if (!objNode.contains("conditions"))
	{
		return true;
	}
	if (!objNode["conditions"].is_array())
	{
		return SetError(errorOut, PathPrefix(path) + "objective.conditions must be an array");
	}

	for (const auto& c : objNode["conditions"])
	{
		if (!c.is_object())
		{
			return SetError(errorOut, PathPrefix(path) + "objective.conditions entry must be a JSON object");
		}
		ObjectiveCondition cond;
		cond.type = c.value("type", std::string{});
		cond.name = c.value("name", std::string{});

		// v1 allow-list. Add new condition types here when their matcher is
		// wired in ObjectiveSystem::Consume.
		if (cond.type != "enemy_killed")
		{
			return SetError(errorOut, PathPrefix(path) + "objective.conditions: unknown type '" + cond.type + "' (expected 'enemy_killed')");
		}
		out.conditions.push_back(std::move(cond));
	}
	return true;
}

// ---- public interface -------------------------------------------------------

// Prefixes every error message with the source file path so multi-scene /
// multi-prefab setups can identify the offender at a glance.
static std::string PathPrefix(const std::filesystem::path& path)
{
	return path.string() + ": ";
}

// Shared parse: file -> JSON -> schemaVersion check. Returns false on failure.
static bool ReadSceneRoot(const std::filesystem::path& path, json& root, std::string* errorOut)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		return SetError(errorOut, PathPrefix(path) + "cannot open file");
	}

	try
	{
		root = json::parse(file);
	}
	catch (const json::parse_error& e)
	{
		return SetError(errorOut, PathPrefix(path) + "JSON parse error: " + e.what());
	}

	if (!root.contains("schemaVersion"))
	{
		return SetError(errorOut, PathPrefix(path) + "missing schemaVersion");
	}
	const int schemaVersion = root["schemaVersion"].get<int>();
	if (schemaVersion != 1 && schemaVersion != 2)
	{
		return SetError(errorOut, PathPrefix(path) + "unsupported schemaVersion " + std::to_string(schemaVersion) + " (expected 1 or 2)");
	}
	return true;
}

bool SceneLoader::Load(const std::filesystem::path& path, Scene& scene, TopDownCamera& camera, std::string* errorOut, ObjectiveState* objectiveOut)
{
	json root;
	if (!ReadSceneRoot(path, root, errorOut))
	{
		return false;
	}

	const auto& sceneNode = root["scene"];

	// Reset objective to empty defaults; a scene without an "objective" block
	// should clear any prior state (important on mid-play reload).
	if (objectiveOut)
	{
		*objectiveOut = {};
	}
	if (objectiveOut && root.contains("objective"))
	{
		if (!root["objective"].is_object())
		{
			return SetError(errorOut, PathPrefix(path) + "objective must be a JSON object");
		}
		if (!ParseObjective(root["objective"], path, *objectiveOut, errorOut))
		{
			return false;
		}
	}

	scene.Clear();

	// Camera target (optional)
	if (sceneNode.contains("camera") && sceneNode["camera"].contains("target"))
	{
		const auto t = ParseFloat3(sceneNode["camera"]["target"]);
		camera.SetTarget(t);
	}

	// Per-load prefab cache: repeated references share one parse, but state
	// never leaks between Load calls (different scenes, different prefab dirs).
	PrefabLoader::Cache prefabCache;

	// Objects
	for (const auto& objJsonRef : sceneNode.value("objects", json::array()))
	{
		// Copy so prefab merge can write missing top-level keys into it.
		json objJson = objJsonRef;

		if (objJson.contains("prefab"))
		{
			const std::string prefabPath = objJson["prefab"].get<std::string>();
			std::string prefabError;
			if (!PrefabLoader::LoadAndMerge(prefabPath, objJson, prefabCache, &prefabError))
			{
				return SetError(errorOut, PathPrefix(path) + prefabError);
			}
		}

		const std::string name = objJson.value("name", "");
		auto& obj = scene.CreateObject(name);

		if (objJson.contains("transform"))
		{
			ParseTransform(objJson["transform"], obj.transform);
		}
		if (objJson.contains("render"))
		{
			obj.renderComponent = ParseRender(objJson["render"]);
		}
		if (objJson.contains("collision"))
		{
			obj.collisionComponent = ParseCollision(objJson["collision"]);
		}
		if (objJson.contains("combat"))
		{
			obj.combatComponent = ParseCombat(objJson["combat"]);
		}
		if (objJson.contains("trigger"))
		{
			TriggerComponent tc;
			if (!ParseTrigger(objJson["trigger"], path, tc, errorOut))
			{
				return false;
			}
			obj.triggerComponent = std::move(tc);
		}
	}

	return true;
}

bool SceneLoader::Validate(const std::filesystem::path& path, std::string* errorOut)
{
	json root;
	if (!ReadSceneRoot(path, root, errorOut))
	{
		return false;
	}

	// Verify each referenced prefab file exists and parses. We do not
	// validate component field shapes here -- that work happens in Load,
	// which produces the same "<path>: <reason>" format. Validate's job is
	// to catch the most common startup failures (typo in filename, stray
	// comma in JSON, wrong schemaVersion) before the window is created.
	const auto& sceneNode = root.value("scene", json::object());
	for (const auto& obj : sceneNode.value("objects", json::array()))
	{
		if (obj.contains("prefab"))
		{
			const std::string prefabPath = obj["prefab"].get<std::string>();

			std::ifstream prefabFile(prefabPath);
			if (!prefabFile.is_open())
			{
				return SetError(errorOut, PathPrefix(path) + "prefab not found: " + prefabPath);
			}
			try
			{
				json dummy = json::parse(prefabFile);
				(void)dummy;
			}
			catch (const json::parse_error& e)
			{
				return SetError(errorOut, PathPrefix(path) + "prefab '" + prefabPath + "' JSON parse error: " + e.what());
			}
		}

		// Trigger action allow-list check mirrors Load's ParseTrigger, so
		// a typoed "teleport"/"loadscene" is rejected before window creation.
		if (obj.contains("trigger"))
		{
			std::optional<TriggerAction> scratchAction;
			if (!ParseTriggerAction(obj["trigger"], path, scratchAction, errorOut))
			{
				return false;
			}
		}
	}

	// Validate optional objective block: we run the same ParseObjective the
	// real Load path uses, which rejects unknown condition types with a
	// path-prefixed error before the window is ever created.
	if (root.contains("objective"))
	{
		if (!root["objective"].is_object())
		{
			return SetError(errorOut, PathPrefix(path) + "objective must be a JSON object");
		}
		ObjectiveState scratch;
		if (!ParseObjective(root["objective"], path, scratch, errorOut))
		{
			return false;
		}
	}

	return true;
}
