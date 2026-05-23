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
	bool playerDefeated = false;
	std::wstring objectiveText;

	// Interaction prompt — set by InteractionSystem each tick when the
	// player stands within range of an NPC and is NOT already in a
	// dialog. TextRenderer draws a small bottom-center hint when this
	// is non-empty ("[E] Talk to <name>"). The string is the NPC's
	// display name (already widened).
	std::wstring interactPromptName;
	bool         hasInteractPrompt = false;
};
