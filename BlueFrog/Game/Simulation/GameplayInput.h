#pragma once

#include <DirectXMath.h>

struct GameplayInput
{
	DirectX::XMFLOAT2 movementIntent = { 0.0f, 0.0f };
	bool attackQueued = false;
	float orbitDelta = 0.0f;
	float zoomDelta = 0.0f;
	DirectX::XMFLOAT2 mousePosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 viewportSize = { 1.0f, 1.0f };
	bool hasMousePosition = false;
};
