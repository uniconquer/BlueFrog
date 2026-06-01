#pragma once

#include "Quest.h"
#include "QuestRegistry.h"

#include "../../Engine/Events/GameEvent.h"

#include <functional>
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

	// Complete → TurnedIn transition. Returns true when the turn-in
	// actually happened (status was Complete and is now TurnedIn).
	// False when the quest wasn't in Complete state — caller must
	// not apply a reward in that case. The Quest's QuestReward lives
	// on the registry's definition, not here; the caller fetches it
	// via QuestRegistry::Find(questId)->reward when this returns true.
	bool TurnIn(const std::string& questId) noexcept;

	// Consume a batch of game events (typically drained from the
	// EventBus once per tick). For every Active quest, walks each
	// EnemyKilled event against the live conditions, incrementing
	// progress where the names match. Transitions Active → Complete
	// when ConditionsMet flips true.
	void Consume(const std::vector<GameEvent>& events) noexcept;

	// Sync "collect_item" leaves (name = item id) against the current
	// inventory via the supplied count lookup, promoting Active → Complete
	// when all conditions are met. Called each tick by the game (which owns
	// the inventory). Only ever raises progress / promotes; never demotes.
	void SyncCollectProgress(const std::function<int(const std::string&)>& have) noexcept;

	// Total quest entries currently tracked (Active + Complete +
	// TurnedIn). Available quests don't allocate.
	[[nodiscard]] std::size_t TrackedCount() const noexcept { return state_.size(); }

	// Persistence (Phase I-D). One snapshot row per tracked quest.
	// `conditionProgress[i]` = highest progress among the leaves of
	// condition slot i (v1 has no OR groups so this is just the
	// single leaf's progress). RestoreFromSnapshot recreates the
	// runtime state by looking the static def up in the registry
	// and applying status + progress on top.
	struct ConditionProgressEntry
	{
		std::string id;
		int         status; // QuestStatus enum value
		std::vector<int> conditionProgress;
	};
	[[nodiscard]] std::vector<ConditionProgressEntry> SnapshotState() const;
	void RestoreFromSnapshot(const std::vector<ConditionProgressEntry>& entries,
		const QuestRegistry& registry) noexcept;

	// Returns the id of the first quest currently Active or Complete,
	// or empty when none. Useful as a "what should the HUD show right
	// now" hint in the v1 single-tracked-quest model. Iteration order
	// is unordered_map's bucket order — fine for one tracked quest;
	// when FL grows multiple in-flight quests we'll replace this with
	// an explicit "tracked quest" selection.
	[[nodiscard]] std::string FindFirstInFlight() const noexcept;

private:
	struct Runtime
	{
		QuestStatus                     status = QuestStatus::Active;
		std::vector<ObjectiveCondition> conditionsLive;
	};

	std::unordered_map<std::string, Runtime> state_;
};
