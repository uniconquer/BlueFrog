#pragma once

#include "Item.h"

#include <filesystem>
#include <string>

// One .item.json file → one Item. Same shape as QuestLoader: path-
// prefixed errors so the boot-time validator can fail loudly with
// a clear message. Required fields: `id` (non-empty string).
// Everything else is optional with sensible defaults.
//
// JSON schema (v1):
// {
//   "id": "healing_potion",
//   "name": "Healing Potion",
//   "description": "Restores 3 HP when consumed.",
//   "maxStack": 99,
//   "effect": { "heal": 3, "boostMaxHealth": 0 }
// }
namespace ItemLoader
{
	bool Load(const std::filesystem::path& path, Item& out, std::string* errorOut);
}
