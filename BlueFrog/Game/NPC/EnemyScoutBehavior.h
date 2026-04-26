#pragma once

#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Physics/CollisionSystem.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Scene/SceneObject.h"
#include "../Combat/CombatSystem.h"
#include <algorithm>
#include <cmath>

// Stateless scout-style melee behavior. The per-instance cooldown timer
// lives on the CombatComponent so SimpleEnemyController can drive any number
// of scouts with a single behavior object. Movement: chase the player when
// inside chaseRange; melee strike when inside attackRange and cooldown is
// spent.
class EnemyScoutBehavior final
{
public:
	void Update(Scene& scene, SceneObject& player, SceneObject& enemy, float dt, EventBus& bus) const noexcept
	{
		if (!enemy.combatComponent.has_value() || !player.combatComponent.has_value())
		{
			return;
		}

		auto& cc = enemy.combatComponent.value();
		cc.attackCooldownRemaining = std::max(0.0f, cc.attackCooldownRemaining - dt);

		if (!cc.IsAlive())
		{
			UpdateTint(enemy, false);
			return;
		}

		if (!player.combatComponent->IsAlive())
		{
			UpdateTint(enemy, false);
			return;
		}

		const float dx = player.transform.position.x - enemy.transform.position.x;
		const float dz = player.transform.position.z - enemy.transform.position.z;
		const float distance = std::sqrt(dx * dx + dz * dz);
		const bool chasing = distance <= chaseRange;

		UpdateTint(enemy, chasing);

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

		if (cc.attackCooldownRemaining <= 0.0f && CombatSystem::TryMeleeAttack(enemy, player, attackDamage, attackRange + 0.2f, &bus))
		{
			cc.attackCooldownRemaining = attackCooldown;
		}
	}

private:
	static float ComputeYawRadians(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to) noexcept
	{
		return std::atan2(to.x - from.x, to.z - from.z);
	}

	static void UpdateTint(SceneObject& enemy, bool chasing) noexcept
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

		enemy.renderComponent->material->tint = chasing ? DirectX::XMFLOAT3{ 1.0f, 0.50f, 0.42f } : DirectX::XMFLOAT3{ 0.92f, 0.36f, 0.36f };
	}

private:
	static constexpr float moveSpeed = 2.8f;
	static constexpr float chaseRange = 12.0f;
	static constexpr float attackRange = 1.8f;
	static constexpr float attackCooldown = 1.15f;
	static constexpr int   attackDamage = 1;
};
