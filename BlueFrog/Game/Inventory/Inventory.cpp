#include "Inventory.h"

#include <algorithm>

int Inventory::Add(const std::string& itemId, int count, const ItemRegistry* registry) noexcept
{
	if (count <= 0 || itemId.empty()) return 0;

	int& current = items_[itemId];

	int allowed = count;
	if (registry != nullptr)
	{
		if (const Item* def = registry->Find(itemId))
		{
			const int room = std::max(0, def->maxStack - current);
			allowed = std::min(count, room);
		}
	}
	current += allowed;
	if (current <= 0)
	{
		// Defensive: a negative `count` shouldn't ever reach here
		// because of the early return, but make sure we never leak
		// an empty entry.
		items_.erase(itemId);
	}
	return allowed;
}

int Inventory::Take(const std::string& itemId, int count) noexcept
{
	if (count <= 0 || itemId.empty()) return 0;
	auto it = items_.find(itemId);
	if (it == items_.end()) return 0;

	const int taken = std::min(count, it->second);
	it->second -= taken;
	if (it->second <= 0)
	{
		items_.erase(it);
	}
	return taken;
}

int Inventory::Count(const std::string& itemId) const noexcept
{
	auto it = items_.find(itemId);
	return (it == items_.end()) ? 0 : it->second;
}
