#pragma once
#include <DirectXMath.h>

struct CollisionComponent
{
	DirectX::XMFLOAT2 halfExtents = { 0.5f, 0.5f };
	bool blocksMovement = true;
};
