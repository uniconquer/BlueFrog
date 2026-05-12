#include "CombatSystem.h"
#include "../../Engine/Audio/AudioEngine.h"
#include "../../Engine/Events/EventBus.h"
#include <algorithm>
#include <cmath>

bool CombatSystem::TryMeleeAttack(SceneObject& attacker, SceneObject& target, int damage, float range, EventBus* bus, AudioEngine* audio, std::vector<DamagePopup>* popups) noexcept
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

	// Dash i-frames live on the target's combat component. The caller's
	// cooldown should NOT be consumed when a strike is dodged (return
	// false) — the attacker can retry next frame after the i-frame window
	// drops, which is the canonical "rolled past the swing" feel.
	if (target.combatComponent->invulnerable)
	{
		return false;
	}

	ApplyDamage(target, damage, bus, audio, popups);
	return true;
}

void CombatSystem::ApplyDamage(SceneObject& target, int damage, EventBus* bus, AudioEngine* audio, std::vector<DamagePopup>* popups) noexcept
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

	const int oldHealth = target.combatComponent->health;
	target.combatComponent->health = std::max(0, target.combatComponent->health - damage);
	const int appliedDamage = oldHealth - target.combatComponent->health;

	// Floating damage number. Anchored at the target's current position;
	// TextRenderer projects this with the live camera each frame and
	// floats the popup upward as it ages. Skip when applied damage clamped
	// to zero (e.g. target already at 0 HP, defensive guard for callers
	// that re-enter the path after a kill).
	if (popups != nullptr && appliedDamage > 0)
	{
		DamagePopup popup;
		popup.worldPos = target.transform.position;
		popup.worldPos.y += DamagePopupConstants::kSpawnYOffset;
		popup.amount   = appliedDamage;
		popup.age      = 0.0f;
		popups->push_back(popup);
	}

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
