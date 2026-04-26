#pragma once
#include "Material.h"
#include <optional>
#include <string>

enum class RenderMeshType
{
	Cube,
	Plane,
	External, // when set, `meshPath` selects a glTF asset to import
};

struct RenderComponent
{
	RenderMeshType meshType = RenderMeshType::Cube;
	// Populated when meshType == External. Path is resolved against the
	// process working directory at render time, the same way texture paths
	// in Material are resolved.
	std::string             meshPath;
	std::optional<Material> material = std::nullopt;
};
