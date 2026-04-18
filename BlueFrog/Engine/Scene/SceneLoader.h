#pragma once

#include "Scene.h"
#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Game/Objectives/ObjectiveState.h"
#include <filesystem>
#include <string>

class SceneLoader
{
public:
	// Loads a scene from a JSON file. Returns true on success.
	// On failure, fills errorOut (if non-null) with a message prefixed by the
	// source file path and leaves the scene unchanged.
	// `objectiveOut` is optional; when non-null and the scene contains a
	// top-level "objective" block, it is populated with the parsed data.
	// If no "objective" block exists the output is left default-constructed.
	static bool Load(const std::filesystem::path& path, Scene& scene, TopDownCamera& camera, std::string* errorOut = nullptr, ObjectiveState* objectiveOut = nullptr);

	// Dry-run parse: verifies the file exists, JSON parses, schemaVersion is
	// recognized, every referenced prefab file exists and parses, and any
	// optional objective/trigger-action blocks use recognized type strings.
	// Does not mutate Scene/Camera. Intended for startup validation over
	// every shipped scene. Errors carry the same "<path>: <reason>" prefix
	// as Load.
	static bool Validate(const std::filesystem::path& path, std::string* errorOut = nullptr);
};
