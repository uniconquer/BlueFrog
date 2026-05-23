#pragma once

#include "../Objectives/ObjectiveState.h"

#include <string>
#include <vector>

// Quest = the unit of an NPC-given mission in FL. Distinct from
// ObjectiveState (which is a scene-level fixed goal) in that:
//   - a Quest is offered/accepted/completed/turned-in via dialog
//     with a specific NPC.
//   - several quests can be loaded in QuestRegistry; only those an
//     NPC references via npc.questId can be active at runtime.
//   - quests carry per-state dialog lines so the NPC says different
//     things before / during / after.
//
// Conditions reuse ObjectiveCondition (engine-agnostic enough for
// reuse, parser already exists in ObjectiveStateIO). v1 supports the
// same leaf type ("enemy_killed") as scene objectives.
//
// Rewards in v1 are tiny on purpose — heal player + bump max HP. The
// reward layer expands once Phase I-2B lands.

struct QuestReward
{
	int healPlayer     = 0;  // restore this many HP on turn-in (capped at maxHealth)
	int boostMaxHealth = 0;  // permanently raise maxHealth by this much (and heal that delta)
};

enum class QuestStatus
{
	// Default: NPC offers it but player hasn't accepted yet. Talking
	// to the NPC for the first time auto-accepts (v1; future versions
	// can split offer/accept into separate dialog beats).
	Available,
	// Player has accepted; conditions in flight.
	Active,
	// All conditions met; ready to turn in to the giver NPC.
	Complete,
	// Reward collected; quest is done forever.
	TurnedIn,
};

struct Quest
{
	std::string  id;        // matches NpcComponent::questId in scenes
	std::wstring title;     // shown in HUD when Active

	// Reuses ObjectiveCondition from the scene-objective layer.
	// Same allow-list ("enemy_killed", "any" group, count-N).
	std::vector<ObjectiveCondition> conditions;

	std::wstring dialogOffer;     // status Available
	std::wstring dialogActive;    // status Active
	std::wstring dialogComplete;  // status Complete (about to turn in)
	std::wstring dialogTurnedIn;  // status TurnedIn (past tense)

	QuestReward  reward;

	// Helper: are all conditions met right now? Same logic as
	// ObjectiveState::IsComplete — AND across slots, OR within a slot.
	[[nodiscard]] bool ConditionsMet() const noexcept
	{
		if (conditions.empty()) return false; // "no conditions" quest never completes by itself
		for (const auto& c : conditions)
		{
			if (!c.IsMet()) return false;
		}
		return true;
	}
};
