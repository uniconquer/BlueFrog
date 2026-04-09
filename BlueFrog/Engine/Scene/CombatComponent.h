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

	bool IsAlive() const noexcept
	{
		return health > 0;
	}
};
