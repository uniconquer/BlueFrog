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

	// Vertical-layer timers + the jump input buffer tick every frame
	// regardless of branch, so a press just before landing (or just after
	// walking off a ledge — coyote) still registers.
	{
		const bool jumpEdge = input.jumpHeld && !prevJumpHeld;
		prevJumpHeld = input.jumpHeld;
		jumpBufferRemaining  = jumpEdge ? kJumpBufferTime : std::max(0.0f, jumpBufferRemaining - dt);
		coyoteRemaining      = std::max(0.0f, coyoteRemaining - dt);
		landingStunRemaining = std::max(0.0f, landingStunRemaining - dt);
	}

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

			// Riding zeroes the vertical layer — no jumping off horseback
			// in V1, and the dismount path drops the player back onto
			// whatever FloorHeightAt says next tick.
			grounded = true;
			verticalVelocity = 0.0f;

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
		if (PlayerAimSystem::ComputeMouseGroundPoint(input, camera, player->transform.position.y, mouseGroundPoint))
		{
			player->transform.rotation.y = PlayerAimSystem::ComputeYawRadians(player->transform.position, mouseGroundPoint);
		}
		// Gravity keeps running through the stagger — getting knocked off
		// a roof should drop you, not freeze you mid-air (no jumping out
		// of a stagger though, and no mantle: 0 move intent).
		IntegrateVertical(*player, scene, dt, /*allowJump=*/false, 0.0f, 0.0f);
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
	if (input.dashHeld && dashCooldownRemaining <= 0.0f && dashTimeRemaining <= 0.0f
		&& grounded && landingStunRemaining <= 0.0f && !mantling)
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
	float effectiveSpeed = (dashTimeRemaining > 0.0f)
		? (moveSpeed * dashSpeedMul)
		: (castingSkill ? 0.0f : moveSpeed);
	// Vertical-layer movement gates: a hard landing roots the player for
	// the stun window; a mantle scripts its own motion; airborne steering
	// keeps most (not all) authority so jump arcs stay predictable.
	if (mantling || landingStunRemaining > 0.0f) effectiveSpeed = 0.0f;
	else if (!grounded)                          effectiveSpeed *= kAirControlMul;
	const float useX = (dashTimeRemaining > 0.0f) ? dashDirX : move.x;
	const float useZ = (dashTimeRemaining > 0.0f) ? dashDirZ : move.z;
	DirectX::XMFLOAT3 desiredPosition = player->transform.position;
	desiredPosition.x += useX * effectiveSpeed * dt;
	desiredPosition.z += useZ * effectiveSpeed * dt;
	CollisionSystem::MoveAndSlide(*player, scene, desiredPosition);

	// Gravity / jump / landing / mantle — after the XZ slide so the floor
	// query sees the final footprint for this tick. Mantle ledge search
	// keys off the move intent, so pass it through.
	IntegrateVertical(*player, scene, dt, /*allowJump=*/!castingSkill, move.x, move.z);

	DirectX::XMFLOAT3 mouseGroundPoint = player->transform.position;
	if (PlayerAimSystem::ComputeMouseGroundPoint(input, camera, player->transform.position.y, mouseGroundPoint))
	{
		player->transform.rotation.y = PlayerAimSystem::ComputeYawRadians(player->transform.position, mouseGroundPoint);
	}

	if (input.attackQueued && skills != nullptr && !mantling)
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
	if (input.heavyAttackQueued && skills != nullptr && !mantling)
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

void PlayerController::IntegrateVertical(SceneObject& player, Scene& scene, float dt, bool allowJump, float moveX, float moveZ) noexcept
{
	DirectX::XMFLOAT3& pos = player.transform.position;

	// Mantle in progress: scripted pull-up, everything else suspended.
	// Y leads X/Z (sy ramps faster) so the body rises over the lip before
	// sliding onto the ledge instead of clipping through the wall.
	if (mantling)
	{
		mantleT += dt / kMantleDuration;
		const float u  = std::clamp(mantleT, 0.0f, 1.0f);
		const float s  = u * u * (3.0f - 2.0f * u);
		const float uy = std::clamp(mantleT * 1.5f, 0.0f, 1.0f);
		const float sy = uy * uy * (3.0f - 2.0f * uy);
		pos.x = mantleFrom.x + (mantleTo.x - mantleFrom.x) * s;
		pos.z = mantleFrom.z + (mantleTo.z - mantleFrom.z) * s;
		pos.y = mantleFrom.y + (mantleTo.y - mantleFrom.y) * sy;
		if (mantleT >= 1.0f)
		{
			pos = mantleTo;
			mantling = false;
			grounded = true;
			verticalVelocity = 0.0f;
		}
		return;
	}

	const float floorY = CollisionSystem::FloorHeightAt(player, scene, pos.x, pos.z, pos.y);

	if (grounded)
	{
		if (pos.y > floorY + kSnapDownHeight)
		{
			// The floor dropped away by more than a step — a real ledge
			// (roof eave, crate edge). Become airborne with no upward
			// kick, and open the coyote window so a slightly-late jump
			// still fires.
			grounded = false;
			verticalVelocity = 0.0f;
			coyoteRemaining = kCoyoteTime;
			airApexY = pos.y;
		}
		else
		{
			// Within a step of the floor: snap straight to it and STAY
			// grounded. Rising = step-up (stairs/crates); falling = the
			// "ground snap" that makes descending stairs / a roof slope
			// read as walking down a ramp instead of a string of little
			// drops (FloorHeightAt already only offers surfaces within
			// +kStepHeight above the feet, so this never teleports the
			// player up a wall).
			pos.y = floorY;
		}
	}

	if (allowJump && jumpBufferRemaining > 0.0f && landingStunRemaining <= 0.0f
		&& (grounded || coyoteRemaining > 0.0f))
	{
		verticalVelocity = kJumpVelocity;
		grounded = false;
		coyoteRemaining = 0.0f;
		jumpBufferRemaining = 0.0f;
		airApexY = pos.y;
	}

	if (!grounded)
	{
		verticalVelocity += kGravity * dt;
		pos.y += verticalVelocity * dt;
		airApexY = std::max(airApexY, pos.y);
		// Re-query at the (possibly moved) XZ: landing surface can differ
		// from takeoff (jumping onto a crate, falling off a roof).
		const float landY = CollisionSystem::FloorHeightAt(player, scene, pos.x, pos.z, std::max(pos.y, airApexY));
		if (verticalVelocity <= 0.0f && pos.y <= landY)
		{
			pos.y = landY;
			grounded = true;
			verticalVelocity = 0.0f;
			const float fallDistance = airApexY - pos.y;
			if (fallDistance >= kHardLandHeight)
			{
				landingStunRemaining = kHardLandStun;
				hardLandedThisTick = true;
			}
		}
		// Mantle: once rising has slowed (at/after apex) and the player is
		// pushing toward a wall, reach for a ledge in the mantle band and
		// pull up. Gated on move intent so it's deliberate — you aim at the
		// ledge — and on vVel so it grabs near the apex, not on the way up.
		else if (verticalVelocity <= 1.0f)
		{
			const float ml = std::sqrt(moveX * moveX + moveZ * moveZ);
			if (ml > 0.1f)
			{
				if (auto ledge = CollisionSystem::FindMantleTarget(player, scene, moveX / ml, moveZ / ml, pos.y))
				{
					mantling   = true;
					mantleT    = 0.0f;
					mantleFrom = pos;
					mantleTo   = *ledge;
				}
			}
		}
	}
}
