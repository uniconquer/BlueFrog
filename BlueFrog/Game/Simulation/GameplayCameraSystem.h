#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "GameplayInput.h"

class GameplayCameraSystem final
{
public:
	void Apply(const GameplayInput& input, TopDownCamera& camera) noexcept
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
};
