#pragma once
#include "GameEvent.h"
#include <vector>

// Tick-drained event queue. Phase 6 v1 semantics:
//   - Publishers call Publish() during GameplaySimulation::Update.
//   - After all systems have run, the simulation calls Drain() exactly once
//     per tick and dispatches events to consumers (ObjectiveSystem,
//     pending-scene-load handler).
//   - Consumers MUST NOT publish while the bus is draining. A debug-build
//     assert guards this — if it ever fires, we are no longer a simple queue
//     and should graduate to a listener-registry model.
//
// This is intentionally NOT a pub/sub with subscriber registration. With
// four systems and three consumers in a single-threaded loop, a plain vector
// drain is both simpler and cheaper. Promotion to a listener model is a
// Phase 7+ concern and would happen wholesale, not in pieces.
class EventBus
{
public:
	void Publish(GameEvent event);

	// Returns the queue contents by value and clears the internal buffer.
	// The returned vector is the sole owner of those events; consumers may
	// iterate freely without worrying about re-entrant publishes (guarded).
	std::vector<GameEvent> Drain();

	// Whether anything is queued. Handy for asserts in tests.
	bool Empty() const noexcept { return queue_.empty(); }

private:
	std::vector<GameEvent> queue_;
	bool draining_ = false;
};
