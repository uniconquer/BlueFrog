#pragma once
#include <DirectXMath.h>

struct CollisionComponent
{
	DirectX::XMFLOAT2 halfExtents = { 0.5f, 0.5f };
	// Local XZ center offset of the box from the object origin, rotated by the
	// object's yaw. Lets the collision box sit under a mesh whose art isn't
	// centered on its origin (e.g. low-poly trees modeled off to one side).
	DirectX::XMFLOAT2 offset = { 0.0f, 0.0f };
	// Vertical extent, local to the object origin (scaled by transform.scale.y
	// then offset by position.y). The defaults reproduce the old behaviour —
	// an infinitely tall pillar — so un-authored scenes/prefabs collide
	// exactly as before. An authored topY turns the box into a 2.5D layer:
	// actors whose body is entirely above topY pass over it, and
	// CollisionSystem::FloorHeightAt reports topY as a walkable surface
	// (GTA2-style roofs/platforms).
	float baseY = 0.0f;
	float topY  = 1.0e9f;
	bool blocksMovement = true;
};
