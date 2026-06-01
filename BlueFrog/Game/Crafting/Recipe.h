#pragma once

#include <string>
#include <vector>

// A single crafting recipe: consume `inputs` (itemId, count) from the
// inventory to produce `outputCount` of `outputItemId`. Loaded from
// Assets/Recipes/*.recipe.json into RecipeRegistry at boot, read-only after.
struct RecipeInput
{
	std::string itemId;
	int         count = 1;
};

struct Recipe
{
	std::string              id;            // unique recipe id
	std::wstring             name;          // shown in the crafting panel
	std::string              outputItemId;  // item produced
	int                      outputCount = 1;
	std::vector<RecipeInput> inputs;        // materials consumed
};
