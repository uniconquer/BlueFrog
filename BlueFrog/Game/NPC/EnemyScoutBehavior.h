#pragma once

#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Physics/CollisionSystem.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Scene/SceneObject.h"
#include "../../Engine/UI/DamagePopup.h"
#include "../Combat/CombatSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

class AudioEngine;

// Stateless scout-style melee behavior. The per-instance cooldown timer
// lives on the CombatComponent so SimpleEnemyController can drive any number
// of scouts with a single behavior object. Movement: chase the player when
// inside chaseRange; melee strike when inside attackRange and cooldown is
// spent.
class EnemyScoutBehavior final
{
public:
	void Update(Scene& scene, SceneObject& player, SceneObject& enemy, float dt, EventBus& bus, AudioEngine* audio, std::vector<DamagePopup>* popups) const noexcept
	{
		if (!enemy.combatComponent.has_value() || !player.combatComponent.has_value())
		{
			return;
		}

		auto& cc = enemy.combatComponent.value();
		cc.attackCooldownRemaining = std::max(0.0f, cc.attackCooldownRemaining - dt);

		if (!cc.IsAlive())
		{
			cc.attackWindupRemaining = 0.0f;
			UpdateTint(enemy, false, 0.0f);
			return;
		}

		if (!player.combatComponent->IsAlive())
		{
			cc.attackWindupRemaining = 0.0f;
			UpdateTint(enemy, false, 0.0f);
			return;
		}

		const float dx = player.transform.position.x - enemy.transform.position.x;
		const float dz = player.transform.position.z - enemy.transform.position.z;
		const float distance = std::sqrt(dx * dx + dz * dz);
		const bool chasing = distance <= chaseRange;

		// Telegraph: while attackWindupRemaining > 0 the scout is committed
		// to the attack. Hold rotation toward the player (the snapshot taken
		// at windup start), flash bright, and fire when the timer expires.
		// If the player slips out of range mid-windup, abort the swing so
		// the scout doesn't whiff against an empty patch of floor (gives the
		// dash some payoff beyond pure i-frames).
		if (cc.attackWindupRemaining > 0.0f)
		{
			cc.attackWindupRemaining = std::max(0.0f, cc.attackWindupRemaining - dt);
			enemy.transform.rotation.y = ComputeYawRadians(enemy.transform.position, player.transform.position);

			if (distance > attackRange + windupAbortSlack)
			{
				// Abort — fall through to chase next tick. Cooldown is NOT
				// consumed because no swing landed (or even released).
				cc.attackWindupRemaining = 0.0f;
				UpdateTint(enemy, chasing, 0.0f);
				return;
			}

			const float windupRatio = (windupDuration > 0.0f)
				? std::clamp(cc.attackWindupRemaining / windupDuration, 0.0f, 1.0f)
				: 0.0f;
			// flashStrength ramps 0 → 1 as windup completes (1 - ratio).
			UpdateTint(enemy, chasing, 1.0f - windupRatio);

			if (cc.attackWindupRemaining <= 0.0f)
			{
				CombatSystem::TryMeleeAttack(enemy, player, attackDamage, attackRange + 0.2f, &bus, audio, popups);
				cc.attackCooldownRemaining = attackCooldown;
			}
			return;
		}

		UpdateTint(enemy, chasing, 0.0f);

		if (!chasing || distance < 0.001f)
		{
			return;
		}

		enemy.transform.rotation.y = ComputeYawRadians(enemy.transform.position, player.transform.position);

		if (distance > attackRange)
		{
			const float invDistance = 1.0f / distance;
			DirectX::XMFLOAT3 desiredPosition = enemy.transform.position;
			desiredPosition.x += dx * invDistance * moveSpeed * dt;
			desiredPosition.z += dz * invDistance * moveSpeed * dt;
			CollisionSystem::MoveAndSlide(enemy, scene, desiredPosition);
			return;
		}

		// In range + cooldown spent → COMMIT to a swing by starting the
		// windup. The actual TryMeleeAttack fires when the windup expires
		// (above), giving the player a readable telegraph window.
		if (cc.attackCooldownRemaining <= 0.0f)
		{
			cc.attackWindupRemaining = windupDuration;
		}
	}

private:
	static float ComputeYawRadians(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to) noexcept
	{
		return std::atan2(to.x - from.x, to.z - from.z);
	}

	// flashStrength in [0,1] drives a lerp from the resting/chasing palette
	// toward a near-white "about to strike" color. Caller passes 0 outside
	// the windup window so this collapses back to the previous behavior.
	static void UpdateTint(SceneObject& enemy, bool chasing, float flashStrength) noexcept
	{
		if (!enemy.renderComponent.has_value() || !enemy.renderComponent->material.has_value())
		{
			return;
		}

		if (!enemy.combatComponent.has_value() || !enemy.combatComponent->IsAlive())
		{
			enemy.renderComponent->material->tint = { 0.30f, 0.32f, 0.36f };
			return;
		}

		const DirectX::XMFLOAT3 base = chasing ? DirectX::XMFLOAT3{ 1.0f, 0.50f, 0.42f } : DirectX::XMFLOAT3{ 0.92f, 0.36f, 0.36f };
		const DirectX::XMFLOAT3 flash = { 1.0f, 0.96f, 0.85f };
		const float t = std::clamp(flashStrength, 0.0f, 1.0f);
		enemy.renderComponent->material->tint =
		{
			base.x + (flash.x - base.x) * t,
			base.y + (flash.y - base.y) * t,
			base.z + (flash.z - base.z) * t,
		};
	}

private:
	static constexpr float moveSpeed = 2.8f;
	static constexpr float chaseRange = 12.0f;
	static constexpr float attackRange = 1.8f;
	static constexpr float attackCooldown = 1.15f;
	static constexpr int   attackDamage = 1;
	// Telegraph window: long enough to read + dash through, short enough
	// not to feel sluggish. windupAbortSlack lets the player just barely
	// out-step the strike (extra grace beyond raw attackRange).
	static constexpr float windupDuration   = 0.35f;
	static constexpr float windupAbortSlack = 0.6f;
};
