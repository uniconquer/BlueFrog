#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Scene/SceneLoader.h"
#include <cstdio>
#include <string>

class GameplayArenaBuilder final
{
public:
	static void Build(Scene& scene, TopDownCamera& camera, const std::string& scenePath) noexcept
	{
		std::string error;
		if (!SceneLoader::Load(scenePath, scene, camera, &error))
		{
			std::fprintf(stderr, "[GameplayArenaBuilder] SceneLoader failed for '%s': %s\n", scenePath.c_str(), error.c_str());
		}
	}
};
