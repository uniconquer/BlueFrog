#include "SkillRegistry.h"

#include "SkillLoader.h"

#include <system_error>

bool SkillRegistry::LoadAll(const std::filesystem::path& directory, std::string* errorOut)
{
	skills_.clear();
	std::error_code ec;
	if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec))
	{
		return true;
	}
	const std::string suffix = ".skill.json";
	for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
	{
		if (ec) break;
		if (!entry.is_regular_file()) continue;
		const std::string n = entry.path().filename().string();
		if (n.size() < suffix.size()) continue;
		if (n.compare(n.size() - suffix.size(), suffix.size(), suffix) != 0) continue;

		Skill s;
		if (!SkillLoader::Load(entry.path(), s, errorOut)) return false;
		if (skills_.count(s.id) > 0)
		{
			if (errorOut) *errorOut = entry.path().string() + ": duplicate skill id '" + s.id + "'";
			return false;
		}
		skills_.emplace(s.id, std::move(s));
	}
	return true;
}

const Skill* SkillRegistry::Find(const std::string& id) const noexcept
{
	auto it = skills_.find(id);
	return (it == skills_.end()) ? nullptr : &it->second;
}
