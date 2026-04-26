#pragma once

#include <string>

// Tags an enemy-faction SceneObject with the AI behavior class that should
// drive it. Parsed from the optional "behavior" block in scene/prefab JSON:
//
//   "behavior": { "type": "scout" }   // chase + melee (default if omitted)
//   "behavior": { "type": "archer" }  // stationary hitscan
//
// Adding a new behavior is: extend SceneLoader's allow-list, add a case to
// SimpleEnemyController::Update's dispatch, and define the behavior class.
// Per-instance state for the behavior lives on CombatComponent (cooldown)
// or as scratch fields on this component itself if a future behavior needs
// more.
struct EnemyBehaviorComponent
{
	std::string type = "scout";
};
