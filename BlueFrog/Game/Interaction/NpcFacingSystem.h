#pragma once

#include "../../Engine/Scene/Scene.h"

// Turns every NPC's yaw toward the player when the player is within
// NoticeRange. Beyond that range NPCs keep whatever rotation the scene
// authored — so distant villagers stay in their scripted poses and
// don't snap-track the player from across the map.
//
// Snap rotation (no smoothing) is intentional for v1: NPCs are static
// in every other way, the player moves slowly enough that the visible
// snap is rare, and a smoothing tween would need a per-NPC state field
// (current yaw vs target yaw) that we don't have a need for yet.
//
// Runs after PlayerGameplaySystem + EnemyGameplaySystem so the player
// position is final for the tick.
namespace NpcFacingSystem
{
	// NPCs within this distance of the player will rotate to face them.
	// A bit larger than InteractRange (2.5) so the "I notice you
	// approaching" turn happens BEFORE the interact prompt appears —
	// the eye contact comes first, then the prompt, then the dialog.
	inline constexpr float NoticeRange = 4.0f;

	void Tick(Scene& scene) noexcept;
}
