#include "CombatSystem.h"
#include <algorithm>
#include <cmath>

bool CombatSystem::TryMeleeAttack(SceneObject& attacker, SceneObject& target, int damage, float range) noexcept
{
	if (!attacker.combatComponent.has_value() || !target.combatComponent.has_value())
	{
		return false;
	}

	if (!attacker.combatComponent->IsAlive() || !target.combatComponent->IsAlive())
	{
		return false;
	}

	if (attacker.combatComponent->faction == target.combatComponent->faction)
	{
		return false;
	}

	if (DistanceXZ(attacker.transform.position, target.transform.position) > range)
	{
		return false;
	}

	ApplyDamage(target, damage);
	return true;
}

void CombatSystem::ApplyDamage(SceneObject& target, int damage) noexcept
{
	if (!target.combatComponent.has_value())
	{
		return;
	}

	target.combatComponent->health = std::max(0, target.combatComponent->health - damage);

	if (!target.renderComponent.has_value() || !target.renderComponent->material.has_value())
	{
		return;
	}

	if (target.combatComponent->IsAlive())
	{
		target.renderComponent->material->tint = { 1.0f, 0.62f, 0.62f };
	}
	else
	{
		target.renderComponent->material->tint = { 0.30f, 0.32f, 0.36f };
		if (target.collisionComponent.has_value())
		{
			target.collisionComponent->blocksMovement = false;
		}
	}
}

float CombatSystem::DistanceXZ(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) noexcept
{
	const float dx = a.x - b.x;
	const float dz = a.z - b.z;
	return std::sqrt(dx * dx + dz * dz);
}
