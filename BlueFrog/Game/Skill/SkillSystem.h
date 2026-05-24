#pragma once

#include "Skill.h"
#include "SkillRegistry.h"

#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/DamagePopup.h"

#include <string>
#include <unordered_map>
#include <vector>

class EventBus;
class AudioEngine;
class ParticleSystem;

// Per-caster skill execution. SkillSystem owns the runtime state
// (current skill id, elapsed time, per-event fired flags) keyed by
// SceneObject name, and ticks it each frame. When an event's `time`
// is crossed, the system fires it: damage events route through
// CombatSystem against the nearest enemy of the caster's faction;
// particle events burst at the caster's position.
class SkillSystem
{
public:
	void BindRegistry(const SkillRegistry& registry) noexcept { registry_ = &registry; }

	bool Start(const std::string& casterObjectName, const std::string& skillId) noexcept;

	[[nodiscard]] bool IsExecuting(const std::string& casterObjectName) const noexcept;

	void Tick(Scene& scene,
		float dt,
		EventBus* bus,
		AudioEngine* audio,
		std::vector<DamagePopup>* popups,
		ParticleSystem* particles) noexcept;

	[[nodiscard]] bool IsCoolingDown(const std::string& casterObjectName,
		const std::string& skillId) const noexcept;

	[[nodiscard]] float CooldownProgress01(const std::string& casterObjectName,
		const std::string& skillId) const noexcept;

private:
	struct Execution
	{
		std::string       skillId;
		float             elapsed = 0.0f;
		std::vector<bool> firedFlags;
	};

	std::unordered_map<std::string, Execution>                              active_;
	std::unordered_map<std::string, std::unordered_map<std::string, float>> cooldowns_;
	const SkillRegistry*                                                    registry_ = nullptr;
};
