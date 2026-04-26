#pragma once

#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Scene/SceneObject.h"
#include "../Combat/CombatSystem.h"
#include <algorithm>
#include <cmath>

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
	void Update(Scene& /*scene*/, SceneObject& player, SceneObject& enemy, float dt, EventBus& bus) const noexcept
	{
		if (!enemy.combatComponent.has_value() || !player.combatComponent.has_value())
		{
			return;
		}

		auto& cc = enemy.combatComponent.value();
		cc.attackCooldownRemaining = std::max(0.0f, cc.attackCooldownRemaining - dt);

		if (!cc.IsAlive() || !player.combatComponent->IsAlive())
		{
			UpdateTint(enemy, 0.0f, /*aware=*/false);
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

		if (inRange && cc.attackCooldownRemaining <= 0.0f)
		{
			// Hitscan: skip the range check inside CombatSystem (already in
			// range), pass a fireRange large enough that TryMeleeAttack's
			// internal range gate doesn't reject. faction-mismatch and
			// alive-checks still run — we want those.
			CombatSystem::TryMeleeAttack(enemy, player, attackDamage, fireRange + 0.5f, &bus);
			cc.attackCooldownRemaining = fireInterval;
		}

		const float fireFlashRatio = (fireInterval > 0.0f)
			? std::clamp(cc.attackCooldownRemaining / fireInterval, 0.0f, 1.0f)
			: 0.0f;
		UpdateTint(enemy, fireFlashRatio, /*aware=*/inRange);
	}

private:
	static void UpdateTint(SceneObject& enemy, float fireFlashRatio, bool aware) noexcept
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
		enemy.renderComponent->material->tint =
		{
			rest.x + (flash.x - rest.x) * fireFlashRatio,
			rest.y + (flash.y - rest.y) * fireFlashRatio,
			rest.z + (flash.z - rest.z) * fireFlashRatio,
		};
	}

private:
	static constexpr float fireRange    = 9.0f;
	static constexpr float fireInterval = 1.6f;
	static constexpr int   attackDamage = 1;
};
