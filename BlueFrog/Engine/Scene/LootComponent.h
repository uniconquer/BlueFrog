#pragma once

#include <string>

// Marks a combat actor as dropping loot when killed. On the alive->dead
// transition the game rolls `chance` (0..1); on success it grants `amount`
// of `itemId` to the player's inventory. Engine-side data only — the game
// decides what the item id means and how it reaches the inventory.
struct LootComponent
{
	std::string itemId;
	int         amount = 1;
	float       chance = 1.0f; // 0..1 probability of dropping on death
};
