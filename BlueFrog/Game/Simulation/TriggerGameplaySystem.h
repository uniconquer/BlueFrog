#pragma once

#include "../../Engine/Scene/Scene.h"

// Checks every trigger volume in the scene against the player position each
// tick. On the first overlap, fires (logs + sets fired=true). Objects without
// a triggerComponent are skipped. The system does not touch collision or
// blocking — triggers are always passable.
class TriggerGameplaySystem final
{
public:
	void Update(Scene& scene) noexcept;
};
