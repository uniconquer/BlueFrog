#include "PlayerController.h"
#include "../../Engine/Audio/AudioEngine.h"
#include "../../Engine/Physics/CollisionSystem.h"
#include "../../Engine/Scene/CombatComponent.h"
#include "PlayerAimSystem.h"
#include "../Simulation/GameplaySceneIds.h"
#include "../Combat/CombatSystem.h"
#include "../Skill/SkillSystem.h"
#include <algorithm>
#include <cmath>

bool PlayerController::Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt, EventBus& bus, AudioEngine* audio, std::vector<DamagePopup>* popups, SkillSystem* skills) noexcept
{
	skillsCached = skills;
	dashTimeRemaining        = std::max(0.0f, dashTimeRemaining - dt);
	dashCooldownRemaining    = std::max(0.0f, dashCooldownRemaining - dt);
	(void)bus; (void)popups; // skill events route through SkillSystem now

	SceneObject* player = FindPlayer(scene);
	if (player == nullptr || !player->combatComponent.has_value())
	{
		return false;
	}

	if (!player->combatComponent->IsAlive())
	{
		UpdateTint(*player);
		return true;
	}

	// Mounted branch: while riding, the player's own position is slaved
	// to the mount, movement input drives the mount's transform, and
	// attacks/dash are disabled. The legacy walk/dash/attack code path
	// is reached only when on foot.
	if (!mountedOnName.empty())
	{
		SceneObject* mount = scene.FindObject(mountedOnName);
		if (mount == nullptr || !mount->mountComponent.has_value())
		{
			// Mount vanished (scene reload, despawn) — fall back to walking.
			mountedOnName.clear();
			if (player->collisionComponent.has_value())
			{
				player->collisionComponent->blocksMovement = true;
			}
		}
		else
		{
			// The rider sits on the mount at the same XZ. If the player
			// keeps blocksMovement=true, the mount's MoveAndSlide will
			// snag on the player every tick and the horse can't budge.
			// Turn off player collision while mounted; restore it on
			// dismount (M3 dismount path below + the early-out above).
			if (player->collisionComponent.has_value())
			{
				player->collisionComponent->blocksMovement = false;
			}

			// GTA-style weighty vehicle controls. WASD picks a TARGET yaw;
			// the mount turns toward it at a finite angular rate. A scalar
			// speed (mountSpeed) ramps up under input and coasts down by
			// friction when released, and the mount always moves along its
			// CURRENT forward axis. Net effect: the horse leans into a
			// curve as it turns and glides to a stop instead of freezing —
			// the heavy, momentum-y feel of GTA1/2 car handling.
			const DirectX::XMFLOAT3 move = PlayerMovementSystem::ComputeMoveVector(input, camera);
			const float moveMagSq = move.x * move.x + move.z * move.z;
			const bool hasInput   = moveMagSq > 0.001f;
			const float topSpeed  = moveSpeed * mount->mountComponent->speedMultiplier;

			float yaw = mount->transform.rotation.y;
			if (hasInput)
			{
				// Turn toward the input direction. Wrap the delta into
				// [-π, π] (atan2 of sin/cos) so a 359°->1° turn goes the
				// short way. Turn rate scales mildly with speed so a
				// near-stopped horse still pivots but a galloping one
				// arcs wider — reads as momentum.
				const float targetYaw = std::atan2(move.x, move.z);
				const float delta = std::atan2(std::sin(targetYaw - yaw),
				                               std::cos(targetYaw - yaw));
				constexpr float kTurnRate = 4.0f; // rad/s
				const float maxStep = kTurnRate * dt;
				yaw += (std::fabs(delta) > maxStep)
					? (delta > 0.0f ? maxStep : -maxStep)
					: delta;
				mount->transform.rotation.y = yaw;

				// Accelerate toward top speed.
				mountSpeed = std::min(mountSpeed + mountAccel * dt, topSpeed);
			}
			else
			{
				// No input: coast down by friction. The horse keeps
				// gliding along its last heading until speed bleeds off.
				mountSpeed = std::max(mountSpeed - mountDecel * dt, 0.0f);
			}

			// Integrate position along the current forward axis at the
			// current speed (zero speed => no move, so a fully stopped
			// horse holds position).
			if (mountSpeed > 0.0f)
			{
				const float fwdX = std::sin(yaw);
				const float fwdZ = std::cos(yaw);
				DirectX::XMFLOAT3 desired = mount->transform.position;
				desired.x += fwdX * mountSpeed * dt;
				desired.z += fwdZ * mountSpeed * dt;
				// Mount keeps its own Y; we don't fly horses.
				CollisionSystem::MoveAndSlide(*mount, scene, desired);
			}

			// Animate off actual speed, not raw input — so the gallop
			// keeps playing through the coast-down and only drops to Idle
			// once the horse has truly stopped.
			if (mount->animationStateComponent.has_value())
			{
				auto& mAsc = mount->animationStateComponent.value();
				mAsc.clipName  = (mountSpeed > 0.1f) ? std::string("Gallop") : std::string("Idle");
				mAsc.playSpeed = 1.0f;
				mAsc.looping   = true;
			}

			// Slave the player to the mount's transform every tick so
			// rendering/skinning/HUD use the player object as the anchor
			// (camera follow, blob shadow, popups already key off player).
			player->transform.position    = mount->transform.position;
			player->transform.position.y += mountRiderYOffset;
			player->transform.rotation.y  = mount->transform.rotation.y;

			player->combatComponent->invulnerable = false;
			UpdateTint(*player);
			return true;
		}
	}

	// On-foot path: idempotently restore collision so dismount cleanup
	// doesn't have to be done by every caller. Cheap — single bool write.
	if (player->collisionComponent.has_value())
	{
		player->collisionComponent->blocksMovement = true;
	}

	// Knockback stun overrides movement / dash start / attack. KnockbackSystem
	// owns the actual slide; this just makes sure the player's own intent
	// doesn't fight it or queue a swing mid-stagger. Aim still updates each
	// tick so the camera-relative facing stays sensible coming out of stun.
	if (player->combatComponent->knockbackTimeRemaining > 0.0f)
	{
		DirectX::XMFLOAT3 mouseGroundPoint = player->transform.position;
		if (PlayerAimSystem::ComputeMouseGroundPoint(input, camera, playerHeight, mouseGroundPoint))
		{
			player->transform.rotation.y = PlayerAimSystem::ComputeYawRadians(player->transform.position, mouseGroundPoint);
		}
		// Clear invulnerable in case the player dashed into this hit — the
		// stun window is a different mechanic (vulnerable) and the dash
		// flag is no longer relevant once knockback is in effect.
		player->combatComponent->invulnerable = (dashTimeRemaining > 0.0f);
		UpdateTint(*player);
		return true;
	}

	const DirectX::XMFLOAT3 move = PlayerMovementSystem::ComputeMoveVector(input, camera);

	// Start a new dash on the first frame Space is held while cooldown is
	// up and we're not already dashing. Direction = current movement
	// vector if any, otherwise the player's current facing yaw projected
	// onto the XZ plane. That way a player who hasn't pressed WASD yet
	// still dashes forward instead of standing still.
	if (input.dashHeld && dashCooldownRemaining <= 0.0f && dashTimeRemaining <= 0.0f)
	{
		float dirX = move.x;
		float dirZ = move.z;
		const float magSq = dirX * dirX + dirZ * dirZ;
		if (magSq < 0.001f)
		{
			const float yaw = player->transform.rotation.y;
			dirX = std::sin(yaw);
			dirZ = std::cos(yaw);
		}
		dashDirX = dirX;
		dashDirZ = dirZ;
		dashTimeRemaining     = dashDuration;
		dashCooldownRemaining = dashDuration + dashCooldown;
	}

	// Dash i-frames: while the dash burst is active the player ignores all
	// incoming damage. Cleared every tick (rather than only at dash end) so a
	// scene reload mid-dash can't strand the player invulnerable forever.
	player->combatComponent->invulnerable = (dashTimeRemaining > 0.0f);

	// Movement lock during skill execution — the player is committed
	// to the swing and shouldn't slide. Dash overrides this since dash
	// is itself a deliberate burst the player chose; cancellation
	// semantics ("dash cancels skill" or vice versa) are a v2 design
	// decision left open.
	const bool castingSkill = (skills != nullptr) && skills->IsExecuting(std::string(GameplaySceneIds::Player));
	const float effectiveSpeed = (dashTimeRemaining > 0.0f)
		? (moveSpeed * dashSpeedMul)
		: (castingSkill ? 0.0f : moveSpeed);
	const float useX = (dashTimeRemaining > 0.0f) ? dashDirX : move.x;
	const float useZ = (dashTimeRemaining > 0.0f) ? dashDirZ : move.z;
	DirectX::XMFLOAT3 desiredPosition = player->transform.position;
	desiredPosition.x += useX * effectiveSpeed * dt;
	desiredPosition.z += useZ * effectiveSpeed * dt;
	desiredPosition.y = playerHeight;
	CollisionSystem::MoveAndSlide(*player, scene, desiredPosition);

	DirectX::XMFLOAT3 mouseGroundPoint = player->transform.position;
	if (PlayerAimSystem::ComputeMouseGroundPoint(input, camera, playerHeight, mouseGroundPoint))
	{
		player->transform.rotation.y = PlayerAimSystem::ComputeYawRadians(player->transform.position, mouseGroundPoint);
	}

	if (input.attackQueued && skills != nullptr)
	{
		// SkillSystem.Start returns false if the skill is mid-execution
		// or still on cooldown — we gate the SFX off the same signal so
		// players aren't spammed with whiff sounds during a held LMB.
		// Pass &scene so Start can snap the player's animation clip to
		// the skill's clip + reset clipTime for a fresh-frame swing.
		if (skills->Start(std::string(GameplaySceneIds::Player), "slash", &scene))
		{
			if (audio) audio->Play("attack");
		}
	}

	// Second skill slot (F key) — heavy slash. SkillSystem rejects the
	// call if any skill is already mid-execution on this caster, so the
	// two slots can't double-cast on top of each other.
	if (input.heavyAttackQueued && skills != nullptr)
	{
		if (skills->Start(std::string(GameplaySceneIds::Player), "heavy_slash", &scene))
		{
			if (audio) audio->Play("attack");
		}
	}

	UpdateTint(*player);
	return true;
}

float PlayerController::GetAttackCooldownProgress01() const noexcept
{
	// Cooldown is now owned by SkillSystem. PlayerController stays the
	// HUD's familiar door so HudPresenter doesn't grow its own
	// SkillSystem dependency.
	if (skillsCached == nullptr) return 1.0f;
	return skillsCached->CooldownProgress01(std::string(GameplaySceneIds::Player), "slash");
}

float PlayerController::GetHeavyAttackCooldownProgress01() const noexcept
{
	if (skillsCached == nullptr) return 1.0f;
	return skillsCached->CooldownProgress01(std::string(GameplaySceneIds::Player), "heavy_slash");
}

SceneObject* PlayerController::FindPlayer(Scene& scene) noexcept
{
	return scene.FindObject(GameplaySceneIds::Player);
}

void PlayerController::UpdateTint(SceneObject& player) const noexcept
{
	if (!player.renderComponent.has_value() || !player.renderComponent->material.has_value() || !player.combatComponent.has_value())
	{
		return;
	}

	if (!player.combatComponent->IsAlive())
	{
		player.renderComponent->material->tint = { 0.28f, 0.30f, 0.34f };
		return;
	}

	const float healthRatio = static_cast<float>(player.combatComponent->health) / static_cast<float>(std::max(1, player.combatComponent->maxHealth));
	// Tint cooldown ratio derives from the skill system now. Mid-skill
	// or mid-cooldown reads as "1 - progress" so the player visibly
	// shifts color while their swing is recovering.
	const float cooldownProgress = (skillsCached != nullptr)
		? skillsCached->CooldownProgress01(std::string(GameplaySceneIds::Player), "slash")
		: 1.0f;
	const float cooldownRatio = 1.0f - cooldownProgress;
	player.renderComponent->material->tint =
	{
		0.55f + (1.0f - cooldownRatio) * 0.35f,
		0.72f + healthRatio * 0.20f,
		0.30f + healthRatio * 0.25f
	};
}
