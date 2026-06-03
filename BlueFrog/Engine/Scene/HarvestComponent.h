#pragma once

#include <DirectXMath.h>
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

	// Runtime "topple + regrow" animation state. Captured the moment the node
	// is harvested (its authored rest pose) so the depletion tick can lean it
	// over, shrink it away, then pop it back to exactly the authored transform
	// when it regrows. The game drives this from cooldownRemaining; nothing
	// here is serialized.
	bool              animCaptured = false;
	DirectX::XMFLOAT3 baseScale = { 1.0f, 1.0f, 1.0f }; // authored scale at rest
	float             basePitch = 0.0f;                 // authored rotation.x at rest
	bool              baseBlocks = true;                // authored blocksMovement at rest

	[[nodiscard]] bool Ready() const noexcept { return cooldownRemaining <= 0.0f; }
};
