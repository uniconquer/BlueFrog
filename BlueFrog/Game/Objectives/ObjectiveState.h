#pragma once
#include <string>
#include <vector>

// A single leaf-level condition. v1 recognizes one type:
//   "enemy_killed" - increments `progress` for every EnemyKilled event whose
//                    payload name matches `name`. Met when progress >= required.
// `required` defaults to 1 (the v1 single-kill behavior).
struct ObjectiveLeaf
{
	std::string type;
	std::string name;
	int         required = 1;
	int         progress = 0;

	bool IsMet() const noexcept { return progress >= required; }
};

// One slot in the objective's top-level conditions array.
//
// - Size 1 = a plain leaf (the v1 single-condition shape; backward-compatible).
// - Size > 1 = an OR group: ANY met leaf satisfies the slot.
//
// The objective's top-level `conditions` vector is still AND-combined, so the
// full evaluation is `AND_i (OR_j leaves[i][j])` — sufficient for "Kill A AND
// (B OR C)" patterns without dragging in a full expression tree.
struct ObjectiveCondition
{
	std::vector<ObjectiveLeaf> leaves;

	bool IsMet() const noexcept
	{
		if (leaves.empty()) return true; // vacuously met
		for (const auto& l : leaves)
		{
			if (l.IsMet()) return true;
		}
		return false;
	}
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

	// AND-combined across the top-level vector. Empty conditions list =>
	// trivially complete. Each ObjectiveCondition is itself OR-combined
	// across its leaves (size 1 = degenerate OR of one).
	bool IsComplete() const noexcept;
};
