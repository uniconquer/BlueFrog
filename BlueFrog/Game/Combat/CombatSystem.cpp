#include "CombatSystem.h"
#include "../../Engine/Events/EventBus.h"
#include <algorithm>
#include <cmath>

bool CombatSystem::TryMeleeAttack(SceneObject& attacker, SceneObject& target, int damage, float range, EventBus* bus) noexcept
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

	ApplyDamage(target, damage, bus);
	return true;
}

void CombatSystem::ApplyDamage(SceneObject& target, int damage, EventBus* bus) noexcept
{
	if (!target.combatComponent.has_value())
	{
		return;
	}

	// Capture the alive state BEFORE mutation so we can detect the exact
	// alive→dead transition. Post-scan death detection is unreliable here
	// because dead enemies persist in the scene (collision disabled, tint
	// darkened) — we'd republish EnemyKilled every tick they remain.
	const bool wasAlive = target.combatComponent->IsAlive();

	target.combatComponent->health = std::max(0, target.combatComponent->health - damage);

	if (bus && wasAlive && !target.combatComponent->IsAlive())
	{
		bus->Publish({ GameEventType::EnemyKilled, target.name, {} });
	}

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
