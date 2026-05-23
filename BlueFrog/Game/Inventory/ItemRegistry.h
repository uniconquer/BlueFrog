#pragma once

#include "Item.h"

#include <filesystem>
#include <string>
#include <unordered_map>

// Static item-definition store. Same lifecycle as QuestRegistry:
// LoadAll() at boot sweeps a directory of .item.json files and
// registers each by id. Read-only after that — Inventory holds the
// runtime per-player count of each id, ItemRegistry holds the shared
// "what is this thing?" definition.
class ItemRegistry
{
public:
	ItemRegistry() = default;
	ItemRegistry(const ItemRegistry&) = delete;
	ItemRegistry& operator=(const ItemRegistry&) = delete;

	bool LoadAll(const std::filesystem::path& directory, std::string* errorOut);

	[[nodiscard]] const Item* Find(const std::string& id) const noexcept;
	[[nodiscard]] std::size_t Size() const noexcept { return items_.size(); }

private:
	std::unordered_map<std::string, Item> items_;
};
