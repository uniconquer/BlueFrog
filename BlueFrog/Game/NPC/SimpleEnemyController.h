#pragma once

#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Scene/Scene.h"
#include "EnemyScoutBehavior.h"

// Drives every enemy-faction combatant in the scene each tick using a single
// stateless EnemyScoutBehavior. Per-instance cooldowns live on the
// CombatComponent, so adding more enemies is purely a scene/prefab change.
class SimpleEnemyController
{
public:
	void Update(Scene& scene, float dt, EventBus& bus) noexcept;
private:
	EnemyScoutBehavior enemyBehavior;
};
