#pragma once
#include <DirectXMath.h>

struct CollisionComponent
{
	DirectX::XMFLOAT2 halfExtents = { 0.5f, 0.5f };
	// Local XZ center offset of the box from the object origin, rotated by the
	// object's yaw. Lets the collision box sit under a mesh whose art isn't
	// centered on its origin (e.g. low-poly trees modeled off to one side).
	DirectX::XMFLOAT2 offset = { 0.0f, 0.0f };
	bool blocksMovement = true;
};
