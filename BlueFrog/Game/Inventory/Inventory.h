#pragma once

#include "ItemRegistry.h"

#include <string>
#include <unordered_map>

// Per-player runtime ownership: item id → count. Tiny v1 — flat map,
// no equipment slots, no sort order. Used by quest reward turn-ins
// (Add), the inventory UI (iteration), consumable hotkeys (Take).
//
// Lifecycle: owned by FLApp (survives scene reloads). Phase I-3D /
// future commit will hook profile save/load so the inventory
// persists across launches.
class Inventory
{
public:
	// Add `count` of `itemId`. Caller is responsible for ensuring the
	// id exists in ItemRegistry — Inventory only knows ids as opaque
	// strings. Stack cap (Item::maxStack) is enforced when `registry`
	// is non-null; pass nullptr to skip the cap (used in early-init
	// paths). Returns the actual amount added (may be less than
	// `count` if the stack cap was hit).
	int Add(const std::string& itemId, int count, const ItemRegistry* registry = nullptr) noexcept;

	// Remove `count` of `itemId`. Returns the actual amount removed
	// (clamped to current count). Entries reaching zero are erased so
	// `Count()` and iteration reflect "owned items only".
	int Take(const std::string& itemId, int count) noexcept;

	[[nodiscard]] int Count(const std::string& itemId) const noexcept;
	[[nodiscard]] bool Empty() const noexcept { return items_.empty(); }

	// For UI iteration. Stable references valid until next mutation.
	[[nodiscard]] const std::unordered_map<std::string, int>& All() const noexcept { return items_; }

private:
	std::unordered_map<std::string, int> items_;
};
