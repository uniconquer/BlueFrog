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
#include <cmath>
#include <fstream>
#include <random>
#include <vector>

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
		if (m.contains("uvScale")) mat.uvScale       = ParseFloat2(m["uvScale"]);
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
	if (j.contains("offset"))      cc.offset = ParseFloat2(j["offset"]);
	// 2.5D vertical extent (see CollisionComponent). Absent = legacy
	// infinite pillar.
	if (j.contains("baseY"))       cc.baseY = j["baseY"].get<float>();
	if (j.contains("topY"))        cc.topY = j["topY"].get<float>();
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

// Build one SceneObject from a (possibly prefab-referencing) JSON blob and
// append it to the scene. Extracted from Load's object loop so the scatter
// directive can reuse the exact same prefab-merge + component-parse path
// for every procedurally generated instance. `objJson` is taken by value
// because prefab merge writes missing keys into it.
static bool BuildObjectFromJson(Scene& scene, json objJson, const std::filesystem::path& path, PrefabLoader::Cache& prefabCache, std::string* errorOut)
{
	if (objJson.contains("prefab"))
	{
		const std::string prefabPath = objJson["prefab"].get<std::string>();
		std::string prefabError;
		if (!PrefabLoader::LoadAndMerge(prefabPath, objJson, prefabCache, &prefabError))
		{
			return SetError(errorOut, PathPrefix(path) + prefabError);
		}
	}

	// Group prefab (Unity-style node hierarchy): a prefab can declare a
	// "children" array of sub-objects (e.g. a house = floors + walls + roof
	// modules). Each child is expanded into its own scene object with its
	// transform composed onto this container's placement, so the assembly
	// is built from separately-imported (and thus correctly RH->LH
	// converted) modules rather than one pre-joined mesh whose baked
	// rotations the import X-mirror would scramble. The container itself
	// carries no renderable.
	if (objJson.contains("children") && objJson["children"].is_array())
	{
		Transform parentT;
		if (objJson.contains("transform")) ParseTransform(objJson["transform"], parentT);
		const std::string namePrefix = objJson.value("name", std::string("group"));
		const float yaw = parentT.rotation.y;
		const float c = std::cos(yaw), s = std::sin(yaw);
		int idx = 0;
		for (const auto& childRef : objJson["children"])
		{
			json child = childRef;
			Transform local;
			if (child.contains("transform")) ParseTransform(child["transform"], local);
			// World position: parent position + parent-yaw-rotated child
			// local XZ (our yaw convention: forward = (sin, cos)).
			const float wx = parentT.position.x + (local.position.x * c + local.position.z * s);
			const float wz = parentT.position.z + (-local.position.x * s + local.position.z * c);
			const float wy = parentT.position.y + local.position.y;
			child["transform"] = {
				{ "position", { wx, wy, wz } },
				{ "rotation", { local.rotation.x, yaw + local.rotation.y, local.rotation.z } },
				{ "scale",    { parentT.scale.x * local.scale.x, parentT.scale.y * local.scale.y, parentT.scale.z * local.scale.z } }
			};
			// Every child (even ones with an explicit name like "Collide_S_R")
			// gets the container's name as a prefix, so the whole assembly
			// shares one prefix. The placement tool's undo/delete keys remove
			// "<name>" + "<name>_*" as a unit -- without this, a group's
			// explicitly-named collision children would be orphaned on undo.
			const std::string childName = child.contains("name")
				? child["name"].get<std::string>()
				: std::to_string(idx);
			child["name"] = namePrefix + "_" + childName;
			++idx;
			if (!BuildObjectFromJson(scene, child, path, prefabCache, errorOut))
			{
				return false;
			}
		}
		return true;
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
		if (n.contains("questId"))     nc.questId     = n["questId"].get<std::string>();
		obj.npcComponent = std::move(nc);
	}
	if (objJson.contains("mount"))
	{
		const auto& m = objJson["mount"];
		MountComponent mc;
		if (m.contains("displayName"))     mc.displayName     = m["displayName"].get<std::string>();
		if (m.contains("speedMultiplier")) mc.speedMultiplier = m["speedMultiplier"].get<float>();
		obj.mountComponent = std::move(mc);
	}
	if (objJson.contains("harvest"))
	{
		const auto& h = objJson["harvest"];
		HarvestComponent hc;
		if (h.contains("itemId"))     hc.itemId     = h["itemId"].get<std::string>();
		if (h.contains("amount"))     hc.amount     = h["amount"].get<int>();
		if (h.contains("respawnSec")) hc.respawnSec = h["respawnSec"].get<float>();
		obj.harvestComponent = std::move(hc);
	}
	if (objJson.contains("loot"))
	{
		const auto& l = objJson["loot"];
		LootComponent lc;
		if (l.contains("itemId")) lc.itemId = l["itemId"].get<std::string>();
		if (l.contains("amount")) lc.amount = l["amount"].get<int>();
		if (l.contains("chance")) lc.chance = l["chance"].get<float>();
		obj.lootComponent = std::move(lc);
	}
	return true;
}

// Expand one "scatter" directive into N procedurally placed prefab
// instances. The directive is the level-design tool that lets a few lines
// of JSON stand in for hundreds of hand-placed objects:
//
//   { "prefab": "Assets/Prefabs/Tree.prefab.json",
//     "area": [x0, z0, x1, z1],   // rectangle on the ground plane
//     "count": 80,
//     "seed": 42,                  // deterministic: same seed -> same layout
//     "y": 0.0,                    // optional ground height (default 0)
//     "avoidRadius": 1.5,          // optional min spacing between instances
//     "scaleRange": [0.8, 1.3],    // optional uniform scale jitter
//     "randomYaw": true,           // optional random Y rotation
//     "namePrefix": "Tree" }       // optional; defaults to "scatter"
//
// Determinism matters: a fixed seed means the same forest every launch, so
// saves / collisions / quests can rely on stable object positions.
//
// Instances also dodge already-placed blocking geometry (houses, walls):
// a candidate that lands on a hand-placed collider is rejected, and if no
// clear spot is found in the attempt budget the instance is skipped rather
// than spawned inside a building.
static bool BuildScatterEntry(Scene& scene, const json& entry, const std::filesystem::path& path, PrefabLoader::Cache& prefabCache, std::string* errorOut)
{
	if (!entry.contains("prefab"))
	{
		return SetError(errorOut, PathPrefix(path) + "scatter entry missing 'prefab'");
	}
	if (!entry.contains("area") || !entry["area"].is_array() || entry["area"].size() != 4)
	{
		return SetError(errorOut, PathPrefix(path) + "scatter entry needs 'area':[x0,z0,x1,z1]");
	}

	const std::string prefab = entry["prefab"].get<std::string>();
	const float x0 = entry["area"][0].get<float>();
	const float z0 = entry["area"][1].get<float>();
	const float x1 = entry["area"][2].get<float>();
	const float z1 = entry["area"][3].get<float>();
	const int   count = entry.value("count", 0);
	const unsigned seed = entry.value("seed", 1337u);
	const float y = entry.value("y", 0.0f);
	const float avoidRadius = entry.value("avoidRadius", 0.0f);
	const bool  randomYaw = entry.value("randomYaw", false);
	const std::string namePrefix = entry.value("namePrefix", std::string("scatter"));

	float scaleMin = 1.0f, scaleMax = 1.0f;
	if (entry.contains("scaleRange") && entry["scaleRange"].is_array() && entry["scaleRange"].size() == 2)
	{
		scaleMin = entry["scaleRange"][0].get<float>();
		scaleMax = entry["scaleRange"][1].get<float>();
	}

	std::mt19937 rng(seed);
	std::uniform_real_distribution<float> distX(std::min(x0, x1), std::max(x0, x1));
	std::uniform_real_distribution<float> distZ(std::min(z0, z1), std::max(z0, z1));
	std::uniform_real_distribution<float> distYaw(0.0f, 6.2831853f);
	std::uniform_real_distribution<float> distScale(scaleMin, scaleMax);

	// Snapshot the blocking objects already in the scene (hand-placed
	// houses, walls, etc.) so scatter instances can dodge them. Captured
	// before the loop because each placed instance is itself appended to
	// the scene; we only want to avoid the authored geometry, not pile-on
	// against earlier scatter of the same kind (avoidRadius covers that).
	struct Blocker { float x, z, hx, hz; };
	std::vector<Blocker> blockers;
	for (const SceneObject& o : scene.GetObjects())
	{
		if (!o.collisionComponent.has_value() || !o.collisionComponent->blocksMovement) continue;
		blockers.push_back({ o.transform.position.x, o.transform.position.z,
			o.collisionComponent->halfExtents.x, o.collisionComponent->halfExtents.y });
	}
	// Margin keeps props clear of a wall/house face. Because we only test a
	// prop's center point (its mesh extent is unknown to the loader), this
	// has to be generous enough that a wide bush/tree center sitting just
	// outside a wall still doesn't visually overlap it. Per-entry override
	// via "blockerMargin" for cases that need more or less breathing room.
	const float kBlockerMargin = entry.value("blockerMargin", 1.2f);

	// Placed positions for the avoid-overlap check. A simple O(n^2) reject
	// is fine for the few-hundred-instance scale these directives target.
	std::vector<std::pair<float, float>> placed;
	placed.reserve(static_cast<size_t>(std::max(0, count)));
	const float avoidSq = avoidRadius * avoidRadius;

	for (int i = 0; i < count; ++i)
	{
		float px = 0.0f, pz = 0.0f;
		bool ok = true;
		// Up to 24 tries to satisfy spacing + blocker avoidance. Spacing
		// (avoidRadius) is a soft constraint — we place anyway if it can't
		// be met — but overlapping a house/wall is a HARD reject: if every
		// attempt lands on blocking geometry we skip this instance rather
		// than spawn a tree inside a building.
		bool insideBlocker = true;
		for (int attempt = 0; attempt < 24; ++attempt)
		{
			px = distX(rng);
			pz = distZ(rng);
			ok = true;
			insideBlocker = false;
			for (const Blocker& b : blockers)
			{
				if (std::fabs(b.x - px) < b.hx + kBlockerMargin &&
				    std::fabs(b.z - pz) < b.hz + kBlockerMargin)
				{
					insideBlocker = true; break;
				}
			}
			if (insideBlocker) { ok = false; continue; }
			if (avoidSq > 0.0f)
			{
				for (const auto& p : placed)
				{
					const float dx = p.first - px;
					const float dz = p.second - pz;
					if (dx * dx + dz * dz < avoidSq) { ok = false; break; }
				}
			}
			if (ok) break;
		}
		// Couldn't find a spot clear of buildings/walls — drop this one.
		if (insideBlocker) continue;
		placed.emplace_back(px, pz);

		const float s = distScale(rng);
		const float yaw = randomYaw ? distYaw(rng) : 0.0f;

		json instance;
		instance["prefab"] = prefab;
		instance["name"] = namePrefix + "_" + std::to_string(i);
		instance["transform"] = {
			{ "position", { px, y, pz } },
			{ "rotation", { 0.0f, yaw, 0.0f } },
			{ "scale",    { s, s, s } }
		};
		if (!BuildObjectFromJson(scene, instance, path, prefabCache, errorOut))
		{
			return false;
		}
	}
	return true;
}

bool SceneLoader::InstantiatePrefab(Scene& scene, const std::string& prefabPath,
	float x, float y, float z, float yawRadians, const std::string& name, std::string* errorOut)
{
	// Prefab files cached across placements; small JSON, reused freely.
	static PrefabLoader::Cache cache;

	// Reserve headroom BEFORE creating so the append inside BuildObjectFromJson
	// doesn't reallocate the object vector mid-session (which would dangle any
	// SceneObject* held by live systems). The caller (placement mode) also
	// suppresses gameplay interaction so this reserve's one-time move is safe.
	auto& objs = scene.GetObjects();
	if (objs.capacity() < objs.size() + 32) objs.reserve(objs.size() + 256);

	json obj;
	obj["name"] = name;
	obj["prefab"] = prefabPath;
	obj["transform"] = {
		{ "position", { x, y, z } },
		{ "rotation", { 0.0, yawRadians, 0.0 } },
		{ "scale",    { 1.0, 1.0, 1.0 } }
	};
	return BuildObjectFromJson(scene, obj, std::filesystem::path("Assets/Scenes/_runtime"), cache, errorOut);
}

bool SceneLoader::GetPrefabPreviewMesh(const std::string& prefabPath, std::string& meshPathOut, float& importScaleOut)
{
	std::ifstream f(prefabPath);
	if (!f) return false;
	json j;
	try { j = json::parse(f); }
	catch (...) { return false; }
	if (j.contains("children")) return false;             // group prefab: no single mesh
	if (!j.contains("render")) return false;
	const auto& r = j["render"];
	if (!r.contains("meshPath")) return false;             // primitive (cube/plane), not a glTF mesh
	meshPathOut    = r["meshPath"].get<std::string>();
	importScaleOut = r.value("importScale", 1.0f);
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
		if (!BuildObjectFromJson(scene, objJsonRef, path, prefabCache, errorOut))
		{
			return false;
		}
	}

	// Scatter directives: procedurally expand each into many prefab
	// instances. Runs after hand-placed objects so explicit objects keep
	// their authored names / positions and scatter fills the space around
	// them.
	for (const auto& scatterEntry : sceneNode.value("scatter", json::array()))
	{
		if (!BuildScatterEntry(scene, scatterEntry, path, prefabCache, errorOut))
		{
			return false;
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

	// Scatter directives: verify each referenced prefab exists + parses,
	// same as hand-placed prefab objects above. Catches a typo'd forest
	// prefab path before the window opens rather than mid-load.
	for (const auto& entry : sceneNode.value("scatter", json::array()))
	{
		if (!entry.contains("prefab"))
		{
			return SetError(errorOut, PathPrefix(path) + "scatter entry missing 'prefab'");
		}
		const std::string prefabPath = entry["prefab"].get<std::string>();
		std::ifstream prefabFile(prefabPath);
		if (!prefabFile.is_open())
		{
			return SetError(errorOut, PathPrefix(path) + "scatter prefab not found: " + prefabPath);
		}
		try
		{
			json dummy = json::parse(prefabFile);
			(void)dummy;
		}
		catch (const json::parse_error& e)
		{
			return SetError(errorOut, PathPrefix(path) + "scatter prefab '" + prefabPath + "' JSON parse error: " + e.what());
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
