#pragma once

#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Scene/SceneObject.h"
#include "../../Engine/UI/DamagePopup.h"
#include "../Combat/CombatSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

class AudioEngine;

// Stationary ranged "archer". Faces the player, fires a hitscan damage tick
// every fireInterval seconds when the player is within fireRange and alive.
// No projectile entity, no line-of-sight check (v1) — the design payoff is
// "different threat shape than scout" not "full ranged-combat simulation".
//
// Visual feedback: tint pulses bright yellow on the firing tick so the
// player can see when the archer attacks. Decays back to the resting tint
// over the next few frames via the cooldown ratio.
class EnemyArcherBehavior final
{
public:
	void Update(Scene& /*scene*/, SceneObject& player, SceneObject& enemy, float dt, EventBus& bus, AudioEngine* audio, std::vector<DamagePopup>* popups) const noexcept
	{
		if (!enemy.combatComponent.has_value() || !player.combatComponent.has_value())
		{
			return;
		}

		auto& cc = enemy.combatComponent.value();
		cc.attackCooldownRemaining = std::max(0.0f, cc.attackCooldownRemaining - dt);

		if (!cc.IsAlive() || !player.combatComponent->IsAlive())
		{
			cc.attackWindupRemaining = 0.0f;
			UpdateTint(enemy, 0.0f, /*aware=*/false, 0.0f);
			return;
		}

		// Knockback stun: drop the windup if we were drawing the bow and
		// stop firing this tick. KnockbackSystem handles the slide.
		if (cc.knockbackTimeRemaining > 0.0f)
		{
			cc.attackWindupRemaining = 0.0f;
			UpdateTint(enemy, 0.0f, /*aware=*/false, 0.0f);
			return;
		}

		const float dx = player.transform.position.x - enemy.transform.position.x;
		const float dz = player.transform.position.z - enemy.transform.position.z;
		const float distance = std::sqrt(dx * dx + dz * dz);
		const bool inRange = distance <= fireRange;

		// Always face the player so the model orientation tells the story
		// even when nothing is happening yet.
		if (distance > 0.001f)
		{
			enemy.transform.rotation.y = std::atan2(dx, dz);
		}

		// Windup state: archer is drawing the bow. Hold facing (above),
		// flash bright, fire when the timer expires. The longer windup
		// (vs scout) is intentional — ranged enemies should feel slower to
		// commit so the player gets time to break line.
		if (cc.attackWindupRemaining > 0.0f)
		{
			cc.attackWindupRemaining = std::max(0.0f, cc.attackWindupRemaining - dt);
			if (!inRange)
			{
				// Player broke range → abort the draw. Cooldown stays
				// untouched (no shot was loosed).
				cc.attackWindupRemaining = 0.0f;
				UpdateTint(enemy, 0.0f, /*aware=*/false, 0.0f);
				return;
			}

			const float windupRatio = (windupDuration > 0.0f)
				? std::clamp(cc.attackWindupRemaining / windupDuration, 0.0f, 1.0f)
				: 0.0f;
			const float windupFlash = 1.0f - windupRatio;
			UpdateTint(enemy, 0.0f, /*aware=*/true, windupFlash);

			if (cc.attackWindupRemaining <= 0.0f)
			{
				// Loose the arrow. Same hitscan path as before; cooldown
				// kicks now so the next windup waits a full fireInterval.
				CombatSystem::TryMeleeAttack(enemy, player, attackDamage, fireRange + 0.5f, &bus, audio, popups);
				cc.attackCooldownRemaining = fireInterval;
			}
			return;
		}

		if (inRange && cc.attackCooldownRemaining <= 0.0f)
		{
			cc.attackWindupRemaining = windupDuration;
		}

		const float fireFlashRatio = (fireInterval > 0.0f)
			? std::clamp(cc.attackCooldownRemaining / fireInterval, 0.0f, 1.0f)
			: 0.0f;
		UpdateTint(enemy, fireFlashRatio, /*aware=*/inRange, 0.0f);
	}

private:
	// `windupFlash` in [0,1] overrides the resting/firing tint with a near-
	// white draw-the-bow color when the archer is committing to a shot.
	// fireFlashRatio + aware drive the legacy pre-windup tinting; both are
	// ignored while windupFlash > 0.
	static void UpdateTint(SceneObject& enemy, float fireFlashRatio, bool aware, float windupFlash) noexcept
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

		// Resting palette: cool teal so the archer reads visually distinct
		// from the scout's red. When aware (player in range) the resting
		// tint shifts brighter; the firing flash is a yellow burst that
		// fades as cooldown ticks down.
		const DirectX::XMFLOAT3 rest    = aware ? DirectX::XMFLOAT3{ 0.45f, 0.85f, 0.95f } : DirectX::XMFLOAT3{ 0.32f, 0.66f, 0.78f };
		const DirectX::XMFLOAT3 flash   = { 1.00f, 0.95f, 0.40f };
		const DirectX::XMFLOAT3 base =
		{
			rest.x + (flash.x - rest.x) * fireFlashRatio,
			rest.y + (flash.y - rest.y) * fireFlashRatio,
			rest.z + (flash.z - rest.z) * fireFlashRatio,
		};

		if (windupFlash <= 0.0f)
		{
			enemy.renderComponent->material->tint = base;
			return;
		}

		const DirectX::XMFLOAT3 windup = { 1.0f, 1.0f, 0.85f };
		const float t = std::clamp(windupFlash, 0.0f, 1.0f);
		enemy.renderComponent->material->tint =
		{
			base.x + (windup.x - base.x) * t,
			base.y + (windup.y - base.y) * t,
			base.z + (windup.z - base.z) * t,
		};
	}

private:
	static constexpr float fireRange    = 9.0f;
	static constexpr float fireInterval = 1.6f;
	static constexpr int   attackDamage = 1;
	// Archer telegraph longer than scout's — ranged commit should feel
	// slower so the player gets a window to break line of sight.
	static constexpr float windupDuration = 0.5f;
};
