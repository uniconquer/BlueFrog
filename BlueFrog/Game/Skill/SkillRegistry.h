#pragma once

#include "Skill.h"

#include <filesystem>
#include <string>
#include <unordered_map>

class SkillRegistry
{
public:
	SkillRegistry() = default;
	SkillRegistry(const SkillRegistry&) = delete;
	SkillRegistry& operator=(const SkillRegistry&) = delete;

	bool LoadAll(const std::filesystem::path& directory, std::string* errorOut);

	[[nodiscard]] const Skill* Find(const std::string& id) const noexcept;
	[[nodiscard]] std::size_t  Size() const noexcept { return skills_.size(); }

private:
	std::unordered_map<std::string, Skill> skills_;
};
