#pragma once

#include <DirectXMath.h>

struct HealthBar
{
	float centerX = 0.0f;
	float centerY = 0.0f;
	float width = 0.4f;
	float height = 0.05f;
	float ratio = 1.0f;
	DirectX::XMFLOAT3 backgroundTint = { 0.08f, 0.09f, 0.12f };
	DirectX::XMFLOAT3 fillTint = { 0.18f, 0.84f, 0.36f };
};
