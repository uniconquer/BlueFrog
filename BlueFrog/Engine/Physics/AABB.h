#pragma once
#include <DirectXMath.h>
#include <cmath>

struct AABB
{
	DirectX::XMFLOAT2 center = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 halfExtents = { 0.0f, 0.0f };

	bool Intersects(const AABB& other) const noexcept
	{
		return std::fabs(center.x - other.center.x) < (halfExtents.x + other.halfExtents.x) &&
			std::fabs(center.y - other.center.y) < (halfExtents.y + other.halfExtents.y);
	}
};
