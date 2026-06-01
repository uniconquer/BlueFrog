#pragma once

#include <string>

// Marks a SceneObject as a gatherable resource node (life-skill / "Fantasy
// Life" gathering loop). InteractionSystem advertises a "Gather" prompt when
// the node is Ready(); on the interact key the game grants `amount` of
// `itemId` to the player's inventory, then the node goes on cooldown for
// `respawnSec` before it can be harvested again.
//
// Lives in the engine (like NpcComponent/MountComponent) because "an
// approachable object that yields something on a timer" is a generic shape;
// the game decides what the item id means and how the inventory works.
struct HarvestComponent
{
	std::string itemId;             // inventory id granted on harvest
	int         amount     = 1;     // units granted per harvest
	float       respawnSec = 8.0f;  // cooldown before it can be harvested again
	float       cooldownRemaining = 0.0f; // runtime; > 0 means depleted/regrowing

	[[nodiscard]] bool Ready() const noexcept { return cooldownRemaining <= 0.0f; }
};
