#pragma once

#include "ObjectiveState.h"

#include <string>

// JSON ↔ ObjectiveState converters. Owned by the Game layer so the Engine
// (SceneLoader / SceneSerializer) stays generic — it ferries the raw JSON
// text of a scene's "objective" block but never knows what conditions look
// like. ObjectiveStateIO is the boundary where game-specific schema
// (`enemy_killed`, `any` groups, count-N) lives.
//
// Why text in/out instead of exposing nlohmann::json in the header: the
// JSON library is a heavy transitive include we don't want to leak into
// every translation unit that touches the objective system. Raw strings
// keep the public surface narrow at the cost of a parse round-trip per
// load — negligible (objective blocks are tiny).
namespace ObjectiveStateIO
{
	// Parses the JSON text of a scene's "objective" block. Empty input
	// yields a default-constructed state (no text, no conditions).
	// `pathPrefix` (e.g. "Assets/Scenes/arena_trial.json: ") is prepended
	// to every error message so the multi-scene boot validator can
	// identify the offender at a glance. Returns true on success; on
	// failure, fills errorOut (if non-null) and leaves `out` untouched.
	bool ParseJson(const std::string& objectiveBlockJsonText,
		const std::string& pathPrefix,
		ObjectiveState& out,
		std::string* errorOut);

	// Encodes the state back to JSON text suitable for the scene file's
	// "objective" key. Returns an empty string when the state has no
	// authored content (empty text + no conditions) so the caller can
	// omit the key entirely — matches the loader's "absence == empty"
	// equivalence.
	std::string EncodeJson(const ObjectiveState& state);
}
