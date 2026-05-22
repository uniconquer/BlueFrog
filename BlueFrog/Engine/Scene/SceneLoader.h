#pragma once

#include "Scene.h"
#include "../../Engine/Camera/TopDownCamera.h"
#include <filesystem>
#include <string>

class SceneLoader
{
public:
	// Loads a scene from a JSON file. Returns true on success.
	// On failure, fills errorOut (if non-null) with a message prefixed by the
	// source file path and leaves the scene unchanged.
	// `objectiveBlockJsonOut` is optional; when non-null and the scene
	// contains a top-level "objective" block, it is populated with the raw
	// JSON text of that block (dumped with no indentation). The Game-layer
	// caller is responsible for converting that text into its own
	// ObjectiveState via ObjectiveStateIO::ParseJson — keeping the engine
	// agnostic of the game's objective schema. If no "objective" block
	// exists the output is set to an empty string.
	static bool Load(const std::filesystem::path& path, Scene& scene, TopDownCamera& camera, std::string* errorOut = nullptr, std::string* objectiveBlockJsonOut = nullptr);

	// Dry-run parse: verifies the file exists, JSON parses, schemaVersion is
	// recognized, every referenced prefab file exists and parses, and any
	// optional trigger-action blocks use recognized type strings. The
	// "objective" block is only shape-checked (must be a JSON object); deep
	// validation lives in the game layer's ObjectiveStateIO::ParseJson and
	// runs against the full boot scene list inside ValidateAllAssets.
	// Errors carry the same "<path>: <reason>" prefix as Load.
	static bool Validate(const std::filesystem::path& path, std::string* errorOut = nullptr);
};
