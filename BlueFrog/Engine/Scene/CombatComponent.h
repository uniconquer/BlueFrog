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

	bool IsAlive() const noexcept
	{
		return health > 0;
	}
};
