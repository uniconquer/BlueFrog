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

	// Asset-level import scale correction. Multiplied into the model
	// matrix at draw time (uniform scale). Used to bring third-party
	// meshes whose authored sizes are not in BlueFrog's 1-unit-=-1-meter
	// convention back to meter-space WITHOUT touching transform.scale,
	// so transform.scale stays meaningful as "intentional in-game size"
	// (e.g. 1.0 = adult human ~1.7m) and prefabs whose source asset is
	// in cm or kilometers can be normalized once at the prefab level.
	//
	// Does NOT affect CollisionComponent.halfExtents — collision is
	// authored directly in meters at the prefab level. Only the visual
	// mesh is rescaled. If a model's collision should also change with
	// importScale, the prefab author updates halfExtents accordingly.
	float                   importScale = 1.0f;
};
