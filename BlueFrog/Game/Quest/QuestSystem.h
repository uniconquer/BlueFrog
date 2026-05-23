#pragma once

#include "Quest.h"
#include "QuestRegistry.h"

#include "../../Engine/Events/GameEvent.h"

#include <string>
#include <unordered_map>
#include <vector>

// Runtime per-quest state. QuestRegistry owns the static definitions;
// this class owns "is this quest accepted yet, what's the progress
// on each condition, has it been turned in already".
//
// Lifecycle for a single quest:
//   Available  ← initial (NPC offers it, player hasn't engaged)
//   Active     ← Accept() (entered when player first talks to giver)
//   Complete   ← all conditionsLive met (auto on event drain)
//   TurnedIn   ← TurnIn() (entered when player re-talks to giver)
//
// Quests not yet referenced are absent from the map. Status() returns
// Available for those — same as fresh state — so callers don't have
// to special-case "registered but never touched".
class QuestSystem
{
public:
	// Default state for a quest the player has never engaged with.
	[[nodiscard]] QuestStatus Status(const std::string& questId) const noexcept;

	// Returns the live condition vector for an Active or Complete
	// quest (with mutated `progress` counters). nullptr for quests
	// that are Available or absent.
	[[nodiscard]] const std::vector<ObjectiveCondition>* LiveConditions(const std::string& questId) const noexcept;

	// Available → Active transition. Copies the registry's conditions
	// into the runtime map so EnemyKilled events can mutate the
	// per-condition progress without touching the read-only registry.
	// No-op if the quest is past Available (already accepted) or the
	// id isn't in the registry.
	void Accept(const std::string& questId, const QuestRegistry& registry) noexcept;

	// Complete → TurnedIn transition. Returns the Quest's reward (so
	// the caller can apply HP/maxHP changes to the player) when the
	// turn-in actually happened, or an empty QuestReward when the
	// quest wasn't in Complete state.
	[[nodiscard]] QuestReward TurnIn(const std::string& questId) noexcept;

	// Consume a batch of game events (typically drained from the
	// EventBus once per tick). For every Active quest, walks each
	// EnemyKilled event against the live conditions, incrementing
	// progress where the names match. Transitions Active → Complete
	// when ConditionsMet flips true.
	void Consume(const std::vector<GameEvent>& events) noexcept;

	// Total quest entries currently tracked (Active + Complete +
	// TurnedIn). Available quests don't allocate.
	[[nodiscard]] std::size_t TrackedCount() const noexcept { return state_.size(); }

private:
	struct Runtime
	{
		QuestStatus                     status = QuestStatus::Active;
		std::vector<ObjectiveCondition> conditionsLive;
	};

	std::unordered_map<std::string, Runtime> state_;
};
