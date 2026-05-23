#pragma once

#include <string>

// Static definition of an item the game knows about. Loaded from
// Assets/Items/*.item.json into ItemRegistry once at boot; never
// mutated thereafter (the *count* a player owns is runtime state in
// Inventory, separate from the item's definition).
//
// v1 is deliberately tiny: name + description + effects + stack
// limit. No weight, no rarity, no equipment slot, no requirements.
// Those land when there's a specific game scene that needs them.

// What happens when the player consumes the item via the inventory
// hotkey. Members are independent and additive — a future "elixir"
// could heal AND boost max HP at once. Zero = no effect on that
// channel.
struct ItemEffect
{
	int heal           = 0; // restore this many HP, capped at maxHealth
	int boostMaxHealth = 0; // permanently raise maxHealth (and heal the delta)
};

struct Item
{
	std::string  id;          // matches inventory key + quest reward refs
	std::wstring name;        // shown in inventory UI
	std::wstring description; // shown in inventory UI under name
	int          maxStack = 99; // soft cap so we don't overflow on weird drops
	ItemEffect   effect;        // optional — zero-valued means non-consumable
};
