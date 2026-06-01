#pragma once

#include "Recipe.h"

#include <filesystem>
#include <string>
#include <vector>

// Loads + holds every crafting recipe (Assets/Recipes/*.recipe.json),
// swept once at boot like ItemRegistry/QuestRegistry. Recipes are kept in
// load order (sorted by id) so the crafting UI can index them by slot.
class RecipeRegistry
{
public:
	RecipeRegistry() = default;
	RecipeRegistry(const RecipeRegistry&) = delete;
	RecipeRegistry& operator=(const RecipeRegistry&) = delete;

	bool LoadAll(const std::filesystem::path& directory, std::string* errorOut);

	[[nodiscard]] const std::vector<Recipe>& All() const noexcept { return recipes_; }
	[[nodiscard]] std::size_t Size() const noexcept { return recipes_.size(); }

private:
	std::vector<Recipe> recipes_;
};
