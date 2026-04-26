#pragma once

#include "../Camera/TopDownCamera.h"
#include "../../Game/Objectives/ObjectiveState.h"
#include "Scene.h"

#include <filesystem>
#include <string>

// Writes a Scene + camera + objective state back to disk as a v2-schema JSON
// file that SceneLoader::Load will round-trip.
//
// Lossiness notes (v1):
//   - Always writes the *merged* form for objects that were originally
//     loaded via "prefab" reference. The prefab path is dropped on save —
//     scene-side overrides and prefab defaults are flattened into one
//     object body. Loading the saved file works correctly, but the file
//     no longer points back at the prefab. (Tracking field origin during
//     load to preserve prefab refs is a future improvement.)
//   - Writes the *current* runtime state of every component, including
//     gameplay-driven changes: an enemy's HP after combat, a trigger's
//     fired flag, a cleared CombatComponent::attackCooldownRemaining, etc.
//     If you save mid-fight, you persist the fight state. The intended
//     editor workflow is: edit fields via inspector at load time, save
//     before any gameplay-induced mutation has happened.
//   - Objective conditions are written without their `progress` counters
//     (the condition spec is what matters for round-trip), so a saved
//     file always loads with a fresh objective.
//
// Returns true on success. On failure, errorOut (if non-null) gets a path-
// prefixed message matching SceneLoader's error style.
namespace SceneSerializer
{
	bool Save(const std::filesystem::path& path,
		const Scene& scene,
		const TopDownCamera& camera,
		const ObjectiveState& objective,
		std::string* errorOut) noexcept;
}
