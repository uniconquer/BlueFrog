#pragma once

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
