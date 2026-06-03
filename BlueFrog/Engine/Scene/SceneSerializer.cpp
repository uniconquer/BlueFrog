#include "SceneSerializer.h"

#include "AnimationStateComponent.h"
#include "CollisionComponent.h"
#include "CombatComponent.h"
#include "EnemyBehaviorComponent.h"
#include "Material.h"
#include "RenderComponent.h"
#include "SceneObject.h"
#include "Transform.h"
#include "TriggerComponent.h"

#include <nlohmann/json.hpp>

#include <fstream>

using json = nlohmann::json;

namespace
{
	json EncodeFloat3(const DirectX::XMFLOAT3& v)
	{
		return json::array({ v.x, v.y, v.z });
	}

	const char* FactionToString(CombatFaction f)
	{
		switch (f)
		{
		case CombatFaction::Player:  return "player";
		case CombatFaction::Enemy:   return "enemy";
		case CombatFaction::Neutral:
		default:                     return "neutral";
		}
	}

	const char* MeshToString(RenderMeshType m)
	{
		switch (m)
		{
		case RenderMeshType::Plane: return "plane";
		case RenderMeshType::Cube:
		default:                    return "cube";
		}
	}

	const char* SamplerToString(SamplerPreset s)
	{
		switch (s)
		{
		case SamplerPreset::ClampLinear: return "clamp_linear";
		case SamplerPreset::WrapPoint:   return "wrap_point";
		case SamplerPreset::WrapLinear:
		default:                         return "wrap_linear";
		}
	}

	json EncodeMaterial(const Material& m)
	{
		json j = json::object();
		if (!m.texturePath.empty()) j["texture"] = m.texturePath;
		j["tint"]    = json::array({ m.tint.x, m.tint.y, m.tint.z });
		j["sampler"] = SamplerToString(m.sampler);
		if (m.uvScale.x != 1.0f || m.uvScale.y != 1.0f)
		{
			j["uvScale"] = json::array({ m.uvScale.x, m.uvScale.y });
		}
		return j;
	}

	json EncodeRender(const RenderComponent& rc)
	{
		json j = json::object();
		// External mesh: write meshPath only (the loader's meshPath-wins
		// rule means we don't need a redundant "mesh": "external" tag).
		if (rc.meshType == RenderMeshType::External)
		{
			j["meshPath"] = rc.meshPath;
		}
		else
		{
			j["mesh"] = MeshToString(rc.meshType);
		}
		if (rc.material.has_value())
		{
			j["material"] = EncodeMaterial(rc.material.value());
		}
		// Only emit importScale when it deviates from the no-op default,
		// so prefabs / scenes that don't author the field stay diff-clean
		// after a round-trip.
		if (rc.importScale != 1.0f)
		{
			j["importScale"] = rc.importScale;
		}
		return j;
	}

	json EncodeCollision(const CollisionComponent& cc)
	{
		json j = json::object();
		j["halfExtents"] = json::array({ cc.halfExtents.x, cc.halfExtents.y });
		if (cc.offset.x != 0.0f || cc.offset.y != 0.0f)
		{
			j["offset"] = json::array({ cc.offset.x, cc.offset.y });
		}
		j["blocking"]    = cc.blocksMovement;
		return j;
	}

	json EncodeCombat(const CombatComponent& bc)
	{
		json j = json::object();
		j["faction"]   = FactionToString(bc.faction);
		j["maxHp"]     = bc.maxHealth;
		j["currentHp"] = bc.health;
		// attackCooldownRemaining is runtime scratch — not part of the schema.
		return j;
	}

	json EncodeBehavior(const EnemyBehaviorComponent& bc)
	{
		json j = json::object();
		j["type"] = bc.type;
		return j;
	}

	json EncodeAnimation(const AnimationStateComponent& a)
	{
		json j = json::object();
		j["clipName"]  = a.clipName;
		j["clipTime"]  = a.clipTime;
		j["playSpeed"] = a.playSpeed;
		j["looping"]   = a.looping;
		return j;
	}

	json EncodeTrigger(const TriggerComponent& tc)
	{
		json j = json::object();
		j["halfExtents"] = json::array({ tc.halfExtents.x, tc.halfExtents.y });
		j["tag"]         = tc.tag;
		j["fireOnce"]    = tc.fireOnce;
		// `fired` is runtime state; round-trip should produce a fresh trigger.
		if (tc.action.has_value())
		{
			json a = json::object();
			a["type"]  = tc.action->type;
			a["param"] = tc.action->param;
			j["action"] = a;
		}
		return j;
	}

	json EncodeTransform(const Transform& t)
	{
		json j = json::object();
		j["position"] = EncodeFloat3(t.position);
		j["rotation"] = EncodeFloat3(t.rotation);
		j["scale"]    = EncodeFloat3(t.scale);
		return j;
	}

	json EncodeNpc(const NpcComponent& nc)
	{
		// Both core fields always written so a round-trip preserves the
		// exact authored shape; loader treats absence as empty string.
		json j = json::object();
		j["displayName"] = nc.displayName;
		j["dialogText"]  = nc.dialogText;
		// questId only on round-trip when set, so quest-free villagers
		// keep their files clean.
		if (!nc.questId.empty())
		{
			j["questId"] = nc.questId;
		}
		return j;
	}

	json EncodeHarvest(const HarvestComponent& hc)
	{
		json j = json::object();
		j["itemId"]     = hc.itemId;
		j["amount"]     = hc.amount;
		j["respawnSec"] = hc.respawnSec;
		return j;
	}

	json EncodeLoot(const LootComponent& lc)
	{
		json j = json::object();
		j["itemId"] = lc.itemId;
		j["amount"] = lc.amount;
		j["chance"] = lc.chance;
		return j;
	}

	json EncodeSceneObject(const SceneObject& obj)
	{
		json j = json::object();
		if (!obj.name.empty()) j["name"] = obj.name;
		j["transform"] = EncodeTransform(obj.transform);
		if (obj.renderComponent.has_value())   j["render"]    = EncodeRender(obj.renderComponent.value());
		if (obj.collisionComponent.has_value()) j["collision"] = EncodeCollision(obj.collisionComponent.value());
		if (obj.combatComponent.has_value())   j["combat"]    = EncodeCombat(obj.combatComponent.value());
		if (obj.triggerComponent.has_value())  j["trigger"]   = EncodeTrigger(obj.triggerComponent.value());
		if (obj.enemyBehaviorComponent.has_value()) j["behavior"] = EncodeBehavior(obj.enemyBehaviorComponent.value());
		if (obj.animationStateComponent.has_value()) j["animation"] = EncodeAnimation(obj.animationStateComponent.value());
		if (obj.npcComponent.has_value())      j["npc"]       = EncodeNpc(obj.npcComponent.value());
		if (obj.harvestComponent.has_value())  j["harvest"]   = EncodeHarvest(obj.harvestComponent.value());
		if (obj.lootComponent.has_value())     j["loot"]      = EncodeLoot(obj.lootComponent.value());
		return j;
	}

}

namespace SceneSerializer
{
	bool Save(const std::filesystem::path& path,
		const Scene& scene,
		const TopDownCamera& camera,
		const std::string& objectiveBlockJson,
		std::string* errorOut) noexcept
	{
		try
		{
			json root = json::object();
			root["schemaVersion"] = 2;

			// Embed the pre-encoded objective block when the caller has
			// content for us. Empty string = no goal; the loader treats
			// absence and emptiness identically, so we omit the key for a
			// cleaner file.
			if (!objectiveBlockJson.empty())
			{
				try
				{
					root["objective"] = json::parse(objectiveBlockJson);
				}
				catch (const json::parse_error& e)
				{
					if (errorOut) *errorOut = path.string() + ": objective JSON parse error during save: " + e.what();
					return false;
				}
			}

			json sceneNode = json::object();
			sceneNode["camera"] = json::object({ { "target", EncodeFloat3(camera.GetTarget()) } });

			json objects = json::array();
			for (const auto& obj : scene.GetObjects())
			{
				objects.push_back(EncodeSceneObject(obj));
			}
			sceneNode["objects"] = std::move(objects);

			root["scene"] = std::move(sceneNode);

			std::ofstream out(path);
			if (!out.is_open())
			{
				if (errorOut) *errorOut = path.string() + ": cannot open file for write";
				return false;
			}
			out << root.dump(2);
			out << '\n';
			return true;
		}
		catch (const std::exception& e)
		{
			if (errorOut) *errorOut = path.string() + ": serialization failed: " + e.what();
			return false;
		}
	}
}
