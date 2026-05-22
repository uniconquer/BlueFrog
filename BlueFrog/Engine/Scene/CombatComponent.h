#pragma once

#include <DirectXMath.h>

enum class CombatFaction
{
	Neutral,
	Player,
	Enemy,
};

struct CombatComponent
{
	CombatFaction faction = CombatFaction::Neutral;
	int maxHealth = 1;
	int health = 1;

	// Per-instance attack cooldown timer. Lives on the component so multiple
	// enemy combatants can share one stateless behavior class without the
	// controller keeping a side-channel timer-per-name. Player-side cooldown
	// still lives in PlayerController for now.
	float attackCooldownRemaining = 0.0f;

	// Per-instance attack-windup (telegraph) timer. Set to the behavior's
	// windupDuration when an enemy commits to an attack; counted down each
	// tick while the enemy holds rotation and visibly flashes. At
	// attackWindupRemaining <= 0 the behavior fires the actual hit, then
	// pushes the cooldown. Lives on the component for the same shared-state
	// reason as attackCooldownRemaining. Player does not currently use it.
	float attackWindupRemaining = 0.0f;

	// Pending knockback impulse, applied by KnockbackSystem every tick while
	// knockbackTimeRemaining > 0. velocity is in world units per second on
	// the XZ plane; the system MoveAndSlides the owning object by velocity*dt
	// each tick and ticks the timer down. CombatSystem::ApplyDamage seeds
	// both fields based on the attacker→target direction; behaviors check
	// knockbackTimeRemaining and skip their own motion/attack while > 0 so
	// the brief stun reads clearly.
	DirectX::XMFLOAT2 knockbackVelocityXZ = { 0.0f, 0.0f };
	float             knockbackTimeRemaining = 0.0f;

	// Transient damage-immunity flag. Currently driven by PlayerController
	// during the dash window so the player can roll through an incoming
	// strike (i-frames). CombatSystem::TryMeleeAttack short-circuits when
	// the target is invulnerable — the attacker's cooldown is left intact so
	// the swing can re-attempt on the next frame once the i-frame window
	// expires (rather than burning the cooldown on a whiffed hit).
	bool invulnerable = false;

	bool IsAlive() const noexcept
	{
		return health > 0;
	}
};
