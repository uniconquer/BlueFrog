#pragma once

#include <DirectXMath.h>

struct GameplayInput
{
	DirectX::XMFLOAT2 movementIntent = { 0.0f, 0.0f };
	bool attackQueued = false;
	bool dashHeld = false;
	// Edge-triggered (true on the frame E went from up to down). FLApp
	// uses it to enter / exit dialog mode; downstream systems can also
	// consume it for other context-sensitive actions (open chest, read
	// sign) once those exist.
	bool interactPressed = false;
	float orbitDelta = 0.0f;
	float zoomDelta = 0.0f;
	DirectX::XMFLOAT2 mousePosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 viewportSize = { 1.0f, 1.0f };
	bool hasMousePosition = false;
};
