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
	// Skill bar — two slots wired to LMB (slash) and F (heavy_slash).
	// Each is a 0..1 progress: 1.0 = ready, < 1.0 = filling back up from
	// cooldown. TextRenderer draws square slots at the bottom-center with
	// a dim overlay sized to (1 - value).
	float attackCooldown01      = 1.0f; // slot 0: slash (LMB)
	float heavyAttackCooldown01 = 1.0f; // slot 1: heavy_slash (F)
	bool hasTarget = false;
	bool playerDefeated = false;
	std::wstring objectiveText;

	// Interaction prompt — set by InteractionSystem each tick when the
	// player stands within range of an NPC and is NOT already in a
	// dialog. TextRenderer draws a small bottom-center hint when this
	// is non-empty ("[E] Talk to <name>"). The string is the NPC's
	// display name (already widened).
	std::wstring interactPromptName;
	// Verb shown before the target name in the prompt: "Talk to", "Mount",
	// "Open" etc. Renderer formats as "[E] <verb> <name>". Defaults to
	// "Talk to" (the original NPC-only behavior) when not set.
	std::wstring interactPromptVerb;
	bool         hasInteractPrompt = false;
};
