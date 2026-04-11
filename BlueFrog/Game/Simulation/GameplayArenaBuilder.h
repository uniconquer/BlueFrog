#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Scene/SceneLoader.h"
#include <cstdio>

class GameplayArenaBuilder final
{
public:
	static void Build(Scene& scene, TopDownCamera& camera) noexcept
	{
		std::string error;
		if (!SceneLoader::Load("Assets/Scenes/arena_trial.json", scene, camera, &error))
		{
			std::fprintf(stderr, "[GameplayArenaBuilder] SceneLoader failed: %s\n", error.c_str());
		}
	}
};
