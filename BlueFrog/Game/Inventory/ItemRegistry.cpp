#include "ItemRegistry.h"

#include "ItemLoader.h"

#include <system_error>

bool ItemRegistry::LoadAll(const std::filesystem::path& directory, std::string* errorOut)
{
	items_.clear();

	std::error_code ec;
	if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec))
	{
		return true; // empty install is allowed (same policy as QuestRegistry)
	}

	const std::string suffix = ".item.json";
	for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
	{
		if (ec) break;
		if (!entry.is_regular_file()) continue;
		const std::string name = entry.path().filename().string();
		if (name.size() < suffix.size()) continue;
		if (name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) continue;

		Item it;
		if (!ItemLoader::Load(entry.path(), it, errorOut))
		{
			return false;
		}
		if (items_.count(it.id) > 0)
		{
			if (errorOut)
			{
				*errorOut = entry.path().string() + ": duplicate item id '" + it.id + "' already registered";
			}
			return false;
		}
		items_.emplace(it.id, std::move(it));
	}
	return true;
}

const Item* ItemRegistry::Find(const std::string& id) const noexcept
{
	auto it = items_.find(id);
	return (it == items_.end()) ? nullptr : &it->second;
}
