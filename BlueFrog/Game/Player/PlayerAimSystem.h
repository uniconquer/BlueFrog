#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../Simulation/GameplayInput.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>

class PlayerAimSystem final
{
public:
	static bool ComputeMouseGroundPoint(const GameplayInput& input, const TopDownCamera& camera, float groundHeight, DirectX::XMFLOAT3& outPoint) noexcept
	{
		if (!input.hasMousePosition)
		{
			return false;
		}

		const float width = std::max(1.0f, input.viewportSize.x);
		const float height = std::max(1.0f, input.viewportSize.y);
		const float ndcX = (input.mousePosition.x / width) * 2.0f - 1.0f;
		const float ndcY = 1.0f - (input.mousePosition.y / height) * 2.0f;

		const DirectX::XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();
		const DirectX::XMMATRIX inverseViewProjection = DirectX::XMMatrixInverse(nullptr, viewProjection);

		const DirectX::XMVECTOR nearPoint = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), inverseViewProjection);
		const DirectX::XMVECTOR farPoint = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), inverseViewProjection);
		const DirectX::XMVECTOR ray = DirectX::XMVectorSubtract(farPoint, nearPoint);
		const float rayY = DirectX::XMVectorGetY(ray);
		if (std::fabs(rayY) < 0.0001f)
		{
			return false;
		}

		const float t = -DirectX::XMVectorGetY(nearPoint) / rayY;
		if (t < 0.0f)
		{
			return false;
		}

		const DirectX::XMVECTOR worldPoint = DirectX::XMVectorAdd(nearPoint, DirectX::XMVectorScale(ray, t));
		DirectX::XMStoreFloat3(&outPoint, worldPoint);
		outPoint.y = groundHeight;
		return true;
	}

	static float ComputeYawRadians(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to) noexcept
	{
		const float dx = to.x - from.x;
		const float dz = to.z - from.z;
		if (std::fabs(dx) < 0.0001f && std::fabs(dz) < 0.0001f)
		{
			return 0.0f;
		}

		return std::atan2(dx, dz);
	}
};
