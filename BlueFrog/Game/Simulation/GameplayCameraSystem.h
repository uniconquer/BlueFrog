#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "GameplaySceneIds.h"
#include "SystemContext.h"

// The camera runs in two phases around the simulation step:
//   - ApplyInput (pre-sim): fold orbit/zoom input into the camera.
//   - FollowPlayer (post-sim): snap the camera target to the player's
//     post-movement position so the view stays locked.
// Both phases take the same SystemContext and pick out what they need;
// GameplaySimulation invokes them on either side of PlayerGameplaySystem.
class GameplayCameraSystem final
{
public:
	void ApplyInput(const SystemContext& ctx) noexcept
	{
		if (ctx.input.orbitDelta != 0.0f)
		{
			ctx.camera.RotateAroundTarget(ctx.input.orbitDelta);
		}

		if (ctx.input.zoomDelta != 0.0f)
		{
			ctx.camera.AdjustZoom(ctx.input.zoomDelta);
		}
	}

	void FollowPlayer(const SystemContext& ctx) noexcept
	{
		if (const SceneObject* player = ctx.scene.FindObject(GameplaySceneIds::Player))
		{
			ctx.camera.SetTarget(player->transform.position);
		}
	}
};
