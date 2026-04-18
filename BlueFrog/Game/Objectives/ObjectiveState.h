#pragma once
#include <string>
#include <vector>

// A single objective condition. v1 recognizes one type:
//   "enemy_killed" - marks met when an EnemyKilled event arrives whose
//                    payload name matches `name`.
// Forward compatibility: adding "trigger_fired" (match on tag) or
// "reach_zone" is a one-case-in-ObjectiveSystem::Consume extension.
struct ObjectiveCondition
{
	std::string type;
	std::string name;
	bool met = false;
};

// Scene-level objective data. Parsed from the top-level "objective" block
// of a scene JSON file. Scenes without that block get an empty ObjectiveState
// (text empty, conditions empty) which is-complete by vacuous truth — the
// title bar falls back to a blank objective segment.
struct ObjectiveState
{
	std::wstring text;            // shown while any condition is unmet
	std::wstring completionText;  // shown once every condition is met
	std::vector<ObjectiveCondition> conditions;

	// AND-combined. Empty conditions list => trivially complete.
	bool IsComplete() const noexcept;
};
