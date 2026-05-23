#pragma once

#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"

#include <string>

// Per-tick scan: find the closest NPC within InteractRange of the player
// and populate HudState's interaction prompt slot accordingly. The
// system is purely a sensor — it does NOT consume the E key or open the
// dialog. The dialog flow is owned by FLApp (game-side) because what to
// do when the player triggers an interaction is a game-policy decision
// (dialog box, shop UI, quest accept screen…), not engine concern.
//
// Why a separate system instead of folding into PlayerController: the
// "closest interactable" scan is generic enough that future kinds of
// interactable (chests, doors, signposts) can share the same range-
// check path. Putting it next to PlayerController would also mean every
// new interactable kind has to thread through PlayerController, which
// already has plenty going on.
//
// Why this file lives in Game/ and not Engine/: the *trigger* threshold
// for a prompt ("you're close enough to talk") is a game-feel decision,
// and the consumer (dialog UI) is also game-side. Engine ships
// NpcComponent (the data shape) and InteractionSystem is what builds on
// it.
namespace InteractionSystem
{
	// Tunable: how close the player has to stand for the prompt to
	// appear. Slightly larger than the player+NPC collision radii sum
	// so the prompt shows up *before* the player physically bumps into
	// the NPC, which reads as "I can talk to this person from here".
	inline constexpr float InteractRange = 2.5f;

	// Returns the SceneObject pointer of the closest NPC in range, or
	// nullptr. Also populates `hud.interactPromptName` / `hasInteractPrompt`
	// when a candidate is found. When `suppressPrompt` is true the closest
	// NPC is still returned but the HUD prompt is cleared — used while a
	// dialog is already active so we don't double-paint the prompt.
	const SceneObject* Tick(const Scene& scene, HudState& hud, bool suppressPrompt) noexcept;
}
