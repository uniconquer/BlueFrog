#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../Simulation/GameplayInput.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>

class PlayerMovementSystem final
{
public:
	static DirectX::XMFLOAT3 ComputeMoveVector(const GameplayInput& input, const TopDownCamera& camera) noexcept
	{
		float x = std::clamp(input.movementIntent.x, -1.0f, 1.0f);
		float z = std::clamp(input.movementIntent.y, -1.0f, 1.0f);

		const DirectX::XMFLOAT3 cameraTarget = camera.GetTarget();
		const DirectX::XMFLOAT3 cameraPos = camera.GetPosition();
		const float forwardX = cameraTarget.x - cameraPos.x;
		const float forwardZ = cameraTarget.z - cameraPos.z;
		const float forwardLength = std::sqrt(forwardX * forwardX + forwardZ * forwardZ);

		if (forwardLength > 0.0001f)
		{
			const float normalizedForwardX = forwardX / forwardLength;
			const float normalizedForwardZ = forwardZ / forwardLength;
			const float rightX = normalizedForwardZ;
			const float rightZ = -normalizedForwardX;
			const float moveX = rightX * x + normalizedForwardX * z;
			const float moveZ = rightZ * x + normalizedForwardZ * z;

			const float length = std::sqrt(moveX * moveX + moveZ * moveZ);
			if (length > 0.0f)
			{
				return { moveX / length, 0.0f, moveZ / length };
			}
		}

		const float length = std::sqrt(x * x + z * z);
		if (length > 0.0f)
		{
			x /= length;
			z /= length;
		}

		return { x, 0.0f, z };
	}
};
