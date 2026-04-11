#pragma once

#include "Scene.h"
#include "../../Engine/Camera/TopDownCamera.h"
#include <filesystem>
#include <string>

class SceneLoader
{
public:
	// Loads a scene from a JSON file. Returns true on success.
	// On failure, fills errorOut (if non-null) and leaves the scene unchanged.
	static bool Load(const std::filesystem::path& path, Scene& scene, TopDownCamera& camera, std::string* errorOut = nullptr);
};
