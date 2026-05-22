#include "PlayerController.h"
#include "../../Engine/Audio/AudioEngine.h"
#include "../../Engine/Physics/CollisionSystem.h"
#include "../../Engine/Scene/CombatComponent.h"
#include "PlayerAimSystem.h"
#include "../Simulation/GameplaySceneIds.h"
#include "../Combat/CombatSystem.h"
#include <algorithm>
#include <cmath>

bool PlayerController::Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt, EventBus& bus, AudioEngine* audio, std::vector<DamagePopup>* popups) noexcept
{
	attackCooldownRemaining = std::max(0.0f, attackCooldownRemaining - dt);
	dashTimeRemaining        = std::max(0.0f, dashTimeRemaining - dt);
	dashCooldownRemaining    = std::max(0.0f, dashCooldownRemaining - dt);

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

	const float effectiveSpeed = (dashTimeRemaining > 0.0f) ? (moveSpeed * dashSpeedMul) : moveSpeed;
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

	if (input.attackQueued && attackCooldownRemaining <= 0.0f)
	{
		// Play the swing SFX on every cooldown-passed click, even if no
		// target is in range — the sound represents the player swinging,
		// not a successful hit.
		if (audio) audio->Play("attack");
		TryAttack(scene, *player, bus, audio, popups);
		attackCooldownRemaining = attackCooldown;
	}

	UpdateTint(*player);
	return true;
}

float PlayerController::GetAttackCooldownProgress01() const noexcept
{
	if (attackCooldown <= 0.0f)
	{
		return 1.0f;
	}

	return std::clamp(1.0f - (attackCooldownRemaining / attackCooldown), 0.0f, 1.0f);
}

SceneObject* PlayerController::FindPlayer(Scene& scene) noexcept
{
	return scene.FindObject(GameplaySceneIds::Player);
}

bool PlayerController::TryAttack(Scene& scene, SceneObject& player, EventBus& bus, AudioEngine* audio, std::vector<DamagePopup>* popups) noexcept
{
	// Hit the nearest alive enemy combatant within range. Iterating each
	// attack is fine — scenes have a handful of objects, and centralizing
	// "what counts as a valid target" here avoids the hardcoded-name lookup
	// that broke as soon as a scene added a second enemy.
	SceneObject* best = nullptr;
	float bestDistSq = attackRange * attackRange;
	for (SceneObject& obj : scene.GetObjects())
	{
		if (&obj == &player) continue;
		if (!obj.combatComponent.has_value()) continue;
		if (obj.combatComponent->faction != CombatFaction::Enemy) continue;
		if (!obj.combatComponent->IsAlive()) continue;

		const float dx = obj.transform.position.x - player.transform.position.x;
		const float dz = obj.transform.position.z - player.transform.position.z;
		const float distSq = dx * dx + dz * dz;
		if (distSq < bestDistSq)
		{
			best = &obj;
			bestDistSq = distSq;
		}
	}

	if (best == nullptr)
	{
		return false;
	}
	return CombatSystem::TryMeleeAttack(player, *best, attackDamage, attackRange, &bus, audio, popups);
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
	const float cooldownRatio = attackCooldownRemaining > 0.0f ? attackCooldownRemaining / attackCooldown : 0.0f;
	player.renderComponent->material->tint =
	{
		0.55f + (1.0f - cooldownRatio) * 0.35f,
		0.72f + healthRatio * 0.20f,
		0.30f + healthRatio * 0.25f
	};
}
