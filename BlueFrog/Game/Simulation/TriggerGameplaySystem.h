#pragma once

#include "SystemContext.h"

// Checks every trigger volume in the scene against the player position each
// tick. On the first overlap, fires (logs + sets fired=true) and publishes a
// TriggerFired event on the bus for downstream consumers. Objects without a
// triggerComponent are skipped. The system does not touch collision or
// blocking — triggers are always passable.
//
// Does not use ctx.dt or ctx.input — triggers are position-only and
// time-independent — but takes the full SystemContext for signature
// uniformity with the rest of the gameplay systems.
class TriggerGameplaySystem final
{
public:
	void Update(const SystemContext& ctx) noexcept;
};
