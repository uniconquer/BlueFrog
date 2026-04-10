#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "GameplayInput.h"
#include "GameplaySceneIds.h"

class GameplayCameraSystem final
{
public:
	void ApplyInput(const GameplayInput& input, TopDownCamera& camera) noexcept
	{
		if (input.orbitDelta != 0.0f)
		{
			camera.RotateAroundTarget(input.orbitDelta);
		}

		if (input.zoomDelta != 0.0f)
		{
			camera.AdjustZoom(input.zoomDelta);
		}
	}

	void FollowPlayer(const Scene& scene, TopDownCamera& camera) noexcept
	{
		if (const SceneObject* player = scene.FindObject(GameplaySceneIds::Player))
		{
			camera.SetTarget(player->transform.position);
		}
	}
};
