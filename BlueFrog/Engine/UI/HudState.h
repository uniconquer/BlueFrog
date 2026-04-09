#pragma once

#include <algorithm>
#include <string>

struct HudMeter
{
	float current = 0.0f;
	float max = 1.0f;

	float Ratio() const noexcept
	{
		if (max <= 0.0f)
		{
			return 0.0f;
		}

		return std::clamp(current / max, 0.0f, 1.0f);
	}
};

struct HudState
{
	HudMeter playerHealth;
	HudMeter targetHealth;
	float attackCooldown01 = 1.0f;
	bool hasTarget = false;
	std::wstring interactionPrompt;
	std::wstring objectiveText;
};
