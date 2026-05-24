#include "SkillSystem.h"

#include "../../Engine/Render/ParticleSystem.h"
#include "../../Engine/Scene/CombatComponent.h"
#include "../../Engine/Scene/SceneObject.h"
#include "../Combat/CombatSystem.h"

#include <algorithm>
#include <cmath>
#include <DirectXMath.h>

bool SkillSystem::Start(const std::string& casterObjectName, const std::string& skillId, Scene* scene) noexcept
{
	if (registry_ == nullptr) return false;
	if (active_.count(casterObjectName) > 0) return false;
	const Skill* def = registry_->Find(skillId);
	if (def == nullptr) return false;
	if (IsCoolingDown(casterObjectName, skillId)) return false;

	Execution e;
	e.skillId = skillId;
	e.elapsed = 0.0f;
	e.firedFlags.assign(def->events.size(), false);
	active_.emplace(casterObjectName, std::move(e));

	// Snap the caster's animation to the skill's clip from frame 0.
	// Without this the attack would pick up wherever the walk loop
	// currently sat — visibly mid-stride. Safe no-op when the scene
	// pointer is missing or the caster has no animation component.
	if (scene != nullptr && !def->animationClip.empty())
	{
		if (SceneObject* obj = scene->FindObject(casterObjectName))
		{
			if (obj->animationStateComponent.has_value())
			{
				auto& asc = obj->animationStateComponent.value();
				asc.clipName  = def->animationClip;
				asc.clipTime  = 0.0f;
				asc.playSpeed = 1.0f;
				asc.looping   = false; // attacks play once, not loop
			}
		}
	}
	return true;
}

bool SkillSystem::IsExecuting(const std::string& casterObjectName) const noexcept
{
	return active_.count(casterObjectName) > 0;
}

bool SkillSystem::IsCoolingDown(const std::string& casterObjectName, const std::string& skillId) const noexcept
{
	auto cit = cooldowns_.find(casterObjectName);
	if (cit == cooldowns_.end()) return false;
	auto sit = cit->second.find(skillId);
	if (sit == cit->second.end()) return false;
	return sit->second > 0.0f;
}

float SkillSystem::CooldownProgress01(const std::string& casterObjectName, const std::string& skillId) const noexcept
{
	if (registry_ == nullptr) return 1.0f;
	const Skill* def = registry_->Find(skillId);
	if (def == nullptr || def->cooldown <= 0.0f) return 1.0f;
	auto cit = cooldowns_.find(casterObjectName);
	if (cit == cooldowns_.end()) return 1.0f;
	auto sit = cit->second.find(skillId);
	if (sit == cit->second.end() || sit->second <= 0.0f) return 1.0f;
	return std::clamp(1.0f - (sit->second / def->cooldown), 0.0f, 1.0f);
}

void SkillSystem::Tick(Scene& scene, float dt,
	EventBus* bus, AudioEngine* audio,
	std::vector<DamagePopup>* popups, ParticleSystem* particles) noexcept
{
	if (registry_ == nullptr) return;

	// Cooldown decay for every caster, every skill.
	for (auto& [caster, perSkill] : cooldowns_)
	{
		for (auto& [skillId, remaining] : perSkill)
		{
			remaining = std::max(0.0f, remaining - dt);
		}
	}

	if (active_.empty()) return;

	for (auto it = active_.begin(); it != active_.end(); )
	{
		Execution& ex = it->second;
		const Skill* def = registry_->Find(ex.skillId);
		if (def == nullptr) { it = active_.erase(it); continue; }

		SceneObject* caster = scene.FindObject(it->first);
		if (caster == nullptr) { it = active_.erase(it); continue; }

		ex.elapsed += dt;

		for (size_t i = 0; i < def->events.size(); ++i)
		{
			if (ex.firedFlags[i]) continue;
			const SkillEvent& e = def->events[i];
			if (ex.elapsed < e.time) continue;
			ex.firedFlags[i] = true;

			if (e.type == "damage")
			{
				if (!caster->combatComponent.has_value()) continue;
				SceneObject* best = nullptr;
				float bestDistSq = e.range * e.range;
				const CombatFaction casterFac = caster->combatComponent->faction;
				for (SceneObject& obj : scene.GetObjects())
				{
					if (&obj == caster) continue;
					if (!obj.combatComponent.has_value()) continue;
					if (obj.combatComponent->faction == casterFac) continue;
					if (!obj.combatComponent->IsAlive()) continue;
					const float dx = obj.transform.position.x - caster->transform.position.x;
					const float dz = obj.transform.position.z - caster->transform.position.z;
					const float d2 = dx*dx + dz*dz;
					if (d2 < bestDistSq) { best = &obj; bestDistSq = d2; }
				}
				if (best != nullptr)
				{
					CombatSystem::TryMeleeAttack(*caster, *best, e.amount, e.range, bus, audio, popups);
				}
			}
			else if (e.type == "particle" && particles != nullptr)
			{
				DirectX::XMFLOAT3 pos = caster->transform.position;
				pos.y += 0.8f;
				particles->Burst(pos, 5, 1.8f, 0.4f,
					DirectX::XMFLOAT4{ 0.95f, 0.92f, 0.55f, 0.9f },
					DirectX::XMFLOAT4{ 1.0f, 1.0f, 0.7f, 0.0f },
					0.14f);
			}
		}

		if (ex.elapsed >= def->duration)
		{
			cooldowns_[it->first][ex.skillId] = def->cooldown;
			it = active_.erase(it);
		}
		else
		{
			++it;
		}
	}
}
