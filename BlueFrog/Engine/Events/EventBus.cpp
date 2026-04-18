#include "EventBus.h"
#include <cassert>

void EventBus::Publish(GameEvent event)
{
	// Re-entrant publish during a drain would mean a consumer is also a
	// publisher — the tick ordering breaks down because a freshly-published
	// event would be silently dropped (already-drained buffer is gone).
	// Assert loudly in debug so the misuse surfaces immediately.
	assert(!draining_ && "EventBus: Publish during Drain is not allowed in v1");
	queue_.push_back(std::move(event));
}

std::vector<GameEvent> EventBus::Drain()
{
	draining_ = true;
	std::vector<GameEvent> out;
	out.swap(queue_);
	draining_ = false;
	return out;
}
