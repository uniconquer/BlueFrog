#pragma once

#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Scene/SceneObject.h"
#include "../Skill/SkillSystem.h"
#include "GameplayInput.h"
#include "GameplaySceneIds.h"

#include <cmath>
#include <string>

// Maps gameplay state to animation clip names. Runs each tick BEFORE
// AnimationSystem::Tick so the clipName decision is fresh when clipTime
// advances. Stage 4b v1: hard-switches clipName based on a small set of
// hardcoded role-aware rules (player vs enemy faction). State changes do
// not reset clipTime — switching from Idle to Walk picks up mid-cycle.
// Crossfade between clips is a Stage 4 advanced feature deferred until
// the visible benefit is large enough to justify the dual-pose render
// cost.
//
// Role detection:
//   - Player: the SceneObject named GameplaySceneIds::Player.
//   - Enemy: any SceneObject whose CombatComponent.faction is Enemy.
//   - Anything else with an AnimationStateComponent (e.g., the
//     SkinnedTest RiggedSimple): left alone, keeps whatever clipName
//     the scene JSON set.
//
// Player thresholds: tied to GameplayInput.movementIntent magnitude.
// Enemy thresholds: hardcoded distances mirroring EnemyScoutBehavior's
// chase/attack ranges so the visible clip matches the AI behavior.
namespace AnimationControllerSystem
{
	inline void Tick(Scene& scene, const GameplayInput& input, float /*dt*/, const SkillSystem* skills = nullptr) noexcept
	{
		const SceneObject* player = scene.FindObject(GameplaySceneIds::Player);
		if (player == nullptr) return;

		// Distance thresholds for enemy state. Loosely matches
		// EnemyScoutBehavior's attackRange (1.8) and chaseRange (12.0)
		// from Stage 2; we use slightly wider numbers so the animation
		// state doesn't oscillate at the exact AI threshold.
		constexpr float kEnemyAttackDist = 2.5f;
		constexpr float kEnemyChaseDist  = 12.0f;
		constexpr float kPlayerMoveThreshold = 0.05f;

		for (SceneObject& obj : scene.GetObjects())
		{
			if (!obj.animationStateComponent.has_value()) continue;
			auto& asc = obj.animationStateComponent.value();

			// Yield to SkillSystem while the actor is mid-skill — the
			// skill installed its own clipName/clipTime/playSpeed in
			// Start(), and overriding it here would crush the swing
			// animation back to walk/idle on the very next tick.
			if (skills != nullptr && skills->IsExecuting(obj.name)) continue;

			const bool isPlayer = (&obj == player);
			const bool isEnemy = obj.combatComponent.has_value()
				&& obj.combatComponent->faction == CombatFaction::Enemy;

			if (isPlayer)
			{
				const bool dead = obj.combatComponent.has_value() && !obj.combatComponent->IsAlive();
				if (dead)
				{
					// Freeze on whatever clip was playing — playSpeed 0
					// stops AnimationSystem::Tick from advancing clipTime.
					asc.playSpeed = 0.0f;
				}
				else
				{
					asc.playSpeed = 1.0f;
					const float move = std::hypot(input.movementIntent.x, input.movementIntent.y);
					// CesiumMan has a single unnamed clip; FindClip falls
					// back to clip[0] regardless of name. The named state
					// strings are still useful for debugging via the
					// inspector and for the day a multi-clip player asset
					// arrives.
					asc.clipName = (move > kPlayerMoveThreshold) ? std::string("Walk") : std::string("Idle");
				}
			}
			else if (isEnemy)
			{
				const bool dead = obj.combatComponent.has_value() && !obj.combatComponent->IsAlive();
				if (dead)
				{
					asc.playSpeed = 0.0f;
					continue; // keep whatever clipName was last set
				}

				asc.playSpeed = 1.0f;
				const float dx = obj.transform.position.x - player->transform.position.x;
				const float dz = obj.transform.position.z - player->transform.position.z;
				const float dist = std::sqrt(dx * dx + dz * dz);

				if (dist < kEnemyAttackDist)
				{
					asc.clipName = "Run";
				}
				else if (dist < kEnemyChaseDist)
				{
					asc.clipName = "Walk";
				}
				else
				{
					asc.clipName = "Survey";
				}
			}
		}
	}
}
