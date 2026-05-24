#pragma once

#include "Skill.h"

#include <filesystem>
#include <string>

namespace SkillLoader
{
	bool Load(const std::filesystem::path& path, Skill& out, std::string* errorOut);
}
