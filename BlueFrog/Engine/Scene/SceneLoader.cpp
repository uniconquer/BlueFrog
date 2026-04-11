#include "SceneLoader.h"
#include "CollisionComponent.h"
#include "CombatComponent.h"
#include "Material.h"
#include "RenderComponent.h"
#include "SceneObject.h"
#include "Transform.h"
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

// ---- public interface -------------------------------------------------------

bool SceneLoader::Load(const std::filesystem::path& path, Scene& scene, TopDownCamera& camera, std::string* errorOut)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		return SetError(errorOut, "SceneLoader: cannot open file: " + path.string());
	}

	json root;
	try
	{
		root = json::parse(file);
	}
	catch (const json::parse_error& e)
	{
		return SetError(errorOut, std::string("SceneLoader: JSON parse error: ") + e.what());
	}

	if (!root.contains("schemaVersion") || root["schemaVersion"].get<int>() != 1)
	{
		return SetError(errorOut, "SceneLoader: unsupported or missing schemaVersion (expected 1)");
	}

	const auto& sceneNode = root["scene"];

	scene.Clear();

	// Camera target (optional)
	if (sceneNode.contains("camera") && sceneNode["camera"].contains("target"))
	{
		const auto t = ParseFloat3(sceneNode["camera"]["target"]);
		camera.SetTarget(t);
	}

	// Objects
	for (const auto& objJson : sceneNode.value("objects", json::array()))
	{
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
	}

	return true;
}
