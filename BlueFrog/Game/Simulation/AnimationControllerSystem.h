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
	// playerAirborne / playerVerticalVel / playerLanding mirror the
	// PlayerController vertical layer (Jump V1): airborne picks the jump
	// clips (rising = JumpStart's push-off pose, falling = JumpLoop),
	// and the brief landing stun after a hard fall plays JumpLand once.
	inline void Tick(Scene& scene, const GameplayInput& input, float /*dt*/, const SkillSystem* skills = nullptr, bool playerMounted = false,
		bool playerAirborne = false, float playerVerticalVel = 0.0f, bool playerLanding = false, bool playerMantling = false) noexcept
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

			// Death check goes BEFORE the SkillSystem yield so a kill that
			// lands mid-swing still flips us to Die (rather than letting
			// the swing keep playing because IsExecuting is still true on
			// a now-dead caster). Set looping=false + reset clipTime once
			// on entry; AnimationSystem then plays Die through and the
			// sampler pins to the last keyframe.
			const bool isDead = obj.combatComponent.has_value() && !obj.combatComponent->IsAlive();
			if (isDead)
			{
				if (asc.clipName != "Die")
				{
					// clipTime reset handled by AnimationSystem on the
					// clipName change; it also crossfades from whatever
					// pose the actor was in into the death fall.
					asc.clipName  = "Die";
					asc.playSpeed = 1.0f;
					asc.looping   = false;
				}
				// Already in Die clip — leave clipTime advancing; sampler
				// holds the last frame past duration so the body stays down.
				continue;
			}

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
				asc.playSpeed = 1.0f;
				asc.looping = true;
				if (playerMounted)
				{
					// While mounted, the rider sits on the horse — Walk
					// input drives the MOUNT, not the rider's legs. The
					// "Ride" clip is a Mixamo Male Sitting Pose retargeted
					// into Knight.glb (see _tmp_fbximport/run_convert.bat);
					// it's not a true "hands on reins" ride but it bends
					// the knees / lowers the torso enough to read as
					// "sitting on something" rather than "standing on
					// horse like a stick".
					asc.clipName = std::string("Ride");
				}
				else if (playerMantling)
				{
					// Pulling up over a ledge — hold the tucked airborne
					// pose; the move is brief (~0.3s) and ends grounded.
					asc.clipName = std::string("JumpLoop");
				}
				else if (playerAirborne)
				{
					// Rising = the push-off pose, falling = the airborne
					// loop. Both loop (true above) so a long fall doesn't
					// freeze on the clip's last frame.
					asc.clipName = (playerVerticalVel > 0.5f) ? std::string("JumpStart") : std::string("JumpLoop");
				}
				else if (playerLanding)
				{
					// Hard-landing stun window: play the landing crouch
					// once; looping=false pins the recovery pose until the
					// stun releases back to Idle/Walk.
					asc.clipName = std::string("JumpLand");
					asc.looping  = false;
				}
				else
				{
					const float move = std::hypot(input.movementIntent.x, input.movementIntent.y);
					// Clip names match the Universal character NLA tracks
					// (_tmp_fbximport/merge_universal.py): Idle, Walk, Run,
					// Slash, SlashDown, Hit, Die, Ride, Jump*.
					asc.clipName = (move > kPlayerMoveThreshold) ? std::string("Walk") : std::string("Idle");
				}
			}
			else if (isEnemy)
			{
				// Dead enemies were handled by the early isDead branch above.
				asc.playSpeed = 1.0f;
				asc.looping = true;
				const float dx = obj.transform.position.x - player->transform.position.x;
				const float dz = obj.transform.position.z - player->transform.position.z;
				const float dist = std::sqrt(dx * dx + dz * dz);

				// Clip names match Knight.glb (Idle / Walk / Slash / Hit / Die).
				// "Run" and "Survey" were holdovers from the old Fox rig; the
				// new humanoid rig has no Run clip, so we collapse the
				// in-range cases to Idle. Actual attack animation is left for
				// enemy behaviors to trigger via SkillSystem (just like the
				// player does), keeping animation a consequence of intent
				// rather than a function of distance.
				if (dist < kEnemyAttackDist)
				{
					asc.clipName = "Idle"; // hold position, attack anim driven by behavior
				}
				else if (dist < kEnemyChaseDist)
				{
					asc.clipName = "Walk";
				}
				else
				{
					asc.clipName = "Idle";
				}
			}
		}
	}
}
