#pragma once
#include "ObjectiveState.h"
#include "../../Engine/Events/GameEvent.h"
#include <vector>

// Consumes GameEvents from the EventBus drain, toggles matching conditions,
// and exposes the current title-bar text. Owned by GameplaySimulation; state
// is wiped via Reset() on every scene load.
class ObjectiveSystem
{
public:
	// Swap in a fresh ObjectiveState (called after scene load). All prior
	// progress is discarded — scene load is a full reset in Phase 6 v1.
	void Reset(ObjectiveState state) noexcept;

	// Walks the event list once, flipping matching conditions to met.
	// Idempotent: re-publishing the same EnemyKilled event keeps met=true.
	void Consume(const std::vector<GameEvent>& events) noexcept;

	// Returns the completion text if every condition is met, otherwise the
	// in-progress text. Either may be empty if the scene has no objective.
	std::wstring CurrentText() const noexcept;

private:
	ObjectiveState state_;
};
