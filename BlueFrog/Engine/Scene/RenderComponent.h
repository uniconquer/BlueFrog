#pragma once
#include "Material.h"
#include <optional>

enum class RenderMeshType
{
	Cube,
	Plane,
};

struct RenderComponent
{
	RenderMeshType meshType = RenderMeshType::Cube;
	std::optional<Material> material = std::nullopt;
};
