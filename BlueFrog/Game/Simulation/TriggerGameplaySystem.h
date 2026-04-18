#pragma once

#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Scene/Scene.h"

// Checks every trigger volume in the scene against the player position each
// tick. On the first overlap, fires (logs + sets fired=true) and publishes a
// TriggerFired event on the bus for downstream consumers. Objects without a
// triggerComponent are skipped. The system does not touch collision or
// blocking — triggers are always passable.
class TriggerGameplaySystem final
{
public:
	void Update(Scene& scene, EventBus& bus) noexcept;
};
