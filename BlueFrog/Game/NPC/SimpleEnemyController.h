#pragma once

#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Scene/Scene.h"
#include "EnemyArcherBehavior.h"
#include "EnemyScoutBehavior.h"

// Drives every enemy-faction combatant in the scene each tick, dispatching to
// the behavior class that matches the object's EnemyBehaviorComponent::type.
// Per-instance state for each behavior lives on the SceneObject's components
// (combat cooldown), so adding more enemies of any type is purely a scene /
// prefab change — no controller plumbing per instance.
class SimpleEnemyController
{
public:
	void Update(Scene& scene, float dt, EventBus& bus) noexcept;
private:
	EnemyScoutBehavior  scoutBehavior;
	EnemyArcherBehavior archerBehavior;
};
