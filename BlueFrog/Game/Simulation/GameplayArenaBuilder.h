#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Scene/SceneLoader.h"
#include "../Objectives/ObjectiveState.h"
#include <cstdio>
#include <string>

class GameplayArenaBuilder final
{
public:
	// Loads `scenePath` into `scene` + `camera` and, if the scene JSON carries
	// an "objective" block, fills `objective`. On load failure, logs to
	// stderr and leaves the outputs in whatever partially-constructed state
	// SceneLoader produced — callers should avoid relying on `scene` content
	// in that case.
	static void Build(Scene& scene, TopDownCamera& camera, const std::string& scenePath, ObjectiveState& objective) noexcept
	{
		std::string error;
		if (!SceneLoader::Load(scenePath, scene, camera, &error, &objective))
		{
			std::fprintf(stderr, "[GameplayArenaBuilder] SceneLoader failed for '%s': %s\n", scenePath.c_str(), error.c_str());
		}
	}
};
