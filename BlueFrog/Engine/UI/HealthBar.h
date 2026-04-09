#pragma once

#include <algorithm>
#include <DirectXMath.h>

struct HealthBar
{
	float current = 0.0f;
	float max = 1.0f;
	DirectX::XMFLOAT2 center = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 size = { 0.4f, 0.05f };
	DirectX::XMFLOAT3 fillTint = { 0.18f, 0.84f, 0.36f };
	DirectX::XMFLOAT3 backgroundTint = { 0.08f, 0.09f, 0.12f };
	bool visible = true;

	float Ratio() const noexcept
	{
		if (max <= 0.0f)
		{
			return 0.0f;
		}

		return std::clamp(current / max, 0.0f, 1.0f);
	}
};
