#include "SceneLoader.h"
#include "AnimationStateComponent.h"
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

// Forward declaration: PathPrefix (defined near the public interface) is
// called from the per-component parsers (trigger, behavior) defined above
// it; the file would otherwise have to be reshuffled to put PathPrefix
// first. Keeping the declaration here is the lightest fix.
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
	// `meshPath` wins over `mesh` when both are present — an external mesh
	// is opt-in via the explicit path. This keeps every existing scene
	// (which only carries "mesh": "cube"/"plane") loading unchanged.
	if (j.contains("meshPath"))
	{
		rc.meshType = RenderMeshType::External;
		rc.meshPath = j["meshPath"].get<std::string>();
	}
	else if (j.contains("mesh"))
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
	// Optional asset-level uniform scale. Absent or 0 ⇒ keep the
	// default 1.0 (no correction). Zero is also rejected to defaults
	// because authoring it would silently collapse the mesh.
	if (j.contains("importScale"))
	{
		const float s = j["importScale"].get<float>();
		if (s > 0.0f) rc.importScale = s;
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

// v1 enemy-behavior allow-list. Keep in sync with SimpleEnemyController's
// dispatch — adding a new behavior is a new entry here and a new case there.
static bool IsKnownEnemyBehavior(const std::string& s)
{
	return s == "scout" || s == "archer";
}

static bool ParseEnemyBehavior(const json& j, const std::filesystem::path& path, EnemyBehaviorComponent& out, std::string* errorOut)
{
	if (!j.is_object())
	{
		return SetError(errorOut, PathPrefix(path) + "behavior must be a JSON object");
	}
	out.type = j.value("type", std::string{"scout"});
	if (!IsKnownEnemyBehavior(out.type))
	{
		return SetError(errorOut, PathPrefix(path) + "behavior.type: unknown '" + out.type + "' (expected 'scout' or 'archer')");
	}
	return true;
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

bool SceneLoader::Load(const std::filesystem::path& path, Scene& scene, TopDownCamera& camera, std::string* errorOut, std::string* objectiveBlockJsonOut)
{
	json root;
	if (!ReadSceneRoot(path, root, errorOut))
	{
		return false;
	}

	const auto& sceneNode = root["scene"];

	// Export the "objective" block as raw JSON text. The Game-layer caller
	// (GameplayArenaBuilder) feeds this into ObjectiveStateIO::ParseJson to
	// realize an ObjectiveState — the engine never sees the parsed struct.
	// A scene without an "objective" block clears the output to "" so a
	// mid-play reload doesn't leak the previous scene's state.
	if (objectiveBlockJsonOut)
	{
		objectiveBlockJsonOut->clear();
	}
	if (objectiveBlockJsonOut && root.contains("objective"))
	{
		if (!root["objective"].is_object())
		{
			return SetError(errorOut, PathPrefix(path) + "objective must be a JSON object");
		}
		// dump() with no indent — the consumer parses it back immediately,
		// so the compact form is fine and avoids the indent option's cost.
		*objectiveBlockJsonOut = root["objective"].dump();
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
		if (objJson.contains("behavior"))
		{
			EnemyBehaviorComponent bc;
			if (!ParseEnemyBehavior(objJson["behavior"], path, bc, errorOut))
			{
				return false;
			}
			obj.enemyBehaviorComponent = std::move(bc);
		}
		if (objJson.contains("animation"))
		{
			// Animation block is permissive — every field optional, default
			// values match the AnimationStateComponent struct defaults.
			const auto& a = objJson["animation"];
			AnimationStateComponent asc;
			if (a.contains("clipName"))  asc.clipName  = a["clipName"].get<std::string>();
			if (a.contains("clipTime"))  asc.clipTime  = a["clipTime"].get<float>();
			if (a.contains("playSpeed")) asc.playSpeed = a["playSpeed"].get<float>();
			if (a.contains("looping"))   asc.looping   = a["looping"].get<bool>();
			obj.animationStateComponent = std::move(asc);
		}
		if (objJson.contains("npc"))
		{
			// NPC block: every field optional, defaults match struct.
			// displayName defaults to "" (InteractionSystem falls back to
			// the SceneObject name). dialogText defaults to "" (NPC stays
			// approachable but has nothing to say — useful for ambient
			// villagers that are placeholders for future dialog).
			const auto& n = objJson["npc"];
			NpcComponent nc;
			if (n.contains("displayName")) nc.displayName = n["displayName"].get<std::string>();
			if (n.contains("dialogText"))  nc.dialogText  = n["dialogText"].get<std::string>();
			obj.npcComponent = std::move(nc);
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
		if (obj.contains("behavior"))
		{
			EnemyBehaviorComponent scratchBehavior;
			if (!ParseEnemyBehavior(obj["behavior"], path, scratchBehavior, errorOut))
			{
				return false;
			}
		}
	}

	// Shape check only on the optional objective block — the engine no
	// longer knows the condition schema. Deep validation runs in the
	// boot-time asset validator one layer up, which feeds the dumped JSON
	// back through ObjectiveStateIO::ParseJson and reports the same
	// "<path>: <reason>" errors.
	if (root.contains("objective") && !root["objective"].is_object())
	{
		return SetError(errorOut, PathPrefix(path) + "objective must be a JSON object");
	}

	return true;
}
