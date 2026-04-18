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

static TriggerComponent ParseTrigger(const json& j)
{
	TriggerComponent tc;
	if (j.contains("halfExtents")) tc.halfExtents = ParseFloat2(j["halfExtents"]);
	if (j.contains("tag"))         tc.tag         = j["tag"].get<std::string>();
	if (j.contains("fireOnce"))    tc.fireOnce    = j["fireOnce"].get<bool>();
	return tc;
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

bool SceneLoader::Load(const std::filesystem::path& path, Scene& scene, TopDownCamera& camera, std::string* errorOut)
{
	json root;
	if (!ReadSceneRoot(path, root, errorOut))
	{
		return false;
	}

	const auto& sceneNode = root["scene"];

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
			obj.triggerComponent = ParseTrigger(objJson["trigger"]);
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
		if (!obj.contains("prefab"))
		{
			continue;
		}
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

	return true;
}
