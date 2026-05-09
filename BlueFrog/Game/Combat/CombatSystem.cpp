#include "CombatSystem.h"
#include "../../Engine/Audio/AudioEngine.h"
#include "../../Engine/Events/EventBus.h"
#include <algorithm>
#include <cmath>

bool CombatSystem::TryMeleeAttack(SceneObject& attacker, SceneObject& target, int damage, float range, EventBus* bus, AudioEngine* audio) noexcept
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

	ApplyDamage(target, damage, bus, audio);
	return true;
}

void CombatSystem::ApplyDamage(SceneObject& target, int damage, EventBus* bus, AudioEngine* audio) noexcept
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

	const bool justKilled = wasAlive && !target.combatComponent->IsAlive();
	if (bus && justKilled)
	{
		bus->Publish({ GameEventType::EnemyKilled, target.name, {} });
	}
	if (audio)
	{
		// "enemy_hit" plays on every successful damage application; if the
		// blow was the killing one we also fire "enemy_kill" right after so
		// the death note layers over the hit. Both placeholders are short
		// enough that overlap reads as a single beat rather than chord.
		audio->Play("enemy_hit");
		if (justKilled) audio->Play("enemy_kill");
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
