#include "QuestRegistry.h"

#include "QuestLoader.h"

#include <system_error>

bool QuestRegistry::LoadAll(const std::filesystem::path& directory, std::string* errorOut)
{
	quests_.clear();

	std::error_code ec;
	if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec))
	{
		// Missing directory is fine — early-development checkouts or
		// games-without-quests should still boot.
		return true;
	}

	const std::string suffix = ".quest.json";
	for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
	{
		if (ec) break;
		if (!entry.is_regular_file()) continue;
		const std::string name = entry.path().filename().string();
		if (name.size() < suffix.size()) continue;
		if (name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) continue;

		Quest q;
		if (!QuestLoader::Load(entry.path(), q, errorOut))
		{
			return false;
		}
		// Duplicate id is an authoring bug — surface it instead of
		// silently last-write-wins.
		if (quests_.count(q.id) > 0)
		{
			if (errorOut)
			{
				*errorOut = entry.path().string() + ": duplicate quest id '" + q.id + "' already registered";
			}
			return false;
		}
		quests_.emplace(q.id, std::move(q));
	}
	return true;
}

const Quest* QuestRegistry::Find(const std::string& id) const noexcept
{
	auto it = quests_.find(id);
	return (it == quests_.end()) ? nullptr : &it->second;
}
