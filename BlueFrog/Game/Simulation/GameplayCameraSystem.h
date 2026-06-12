#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Physics/CollisionSystem.h"
#include "../../Engine/Scene/Scene.h"
#include "GameplaySceneIds.h"
#include "SystemContext.h"

#include <algorithm>

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
			// Player position is feet-level after Stage 4c. Aim the camera
			// at chest height so the framing centers on the character body
			// rather than the floor under their feet.
			constexpr float kChestOffsetY = 1.0f;
			DirectX::XMFLOAT3 t = player->transform.position;
			// Vertical: follow the FLOOR under the player (smoothed), not
			// the ballistic jump arc — a flat jump must not bob the whole
			// view. Climbing a roof eases the camera up; jumping in place
			// leaves it rock-steady.
			const float floorY = CollisionSystem::FloorHeightAt(
				*player, ctx.scene, t.x, t.z, player->transform.position.y);
			if (!floorYInitialized)
			{
				smoothedFloorY = floorY;
				floorYInitialized = true;
			}
			smoothedFloorY += (floorY - smoothedFloorY) * (std::min)(1.0f, ctx.dt * 6.0f);
			t.y = smoothedFloorY + kChestOffsetY;
			ctx.camera.SetTarget(t);
		}
	}
private:
	float smoothedFloorY    = 0.0f;
	bool  floorYInitialized = false;
};
