#pragma once
#include <string>

// Simple tagged POD event. Three event types cover Phase 6:
//   EnemyKilled        - CombatSystem publishes when a target's HP crosses to 0.
//                        a = target name, b = unused.
//   TriggerFired       - TriggerGameplaySystem publishes when the player first
//                        enters a trigger zone.
//                        a = trigger tag, b = object name (or action param).
//   LoadSceneRequested - A load_scene trigger action, or any system, asks for
//                        a mid-play scene swap.
//                        a = scene path, b = unused.
//
// We deliberately avoid std::variant: event types are few and each payload is
// <= two strings. A tagged POD keeps the queue trivially copyable and the
// consumer code branch-on-enum, matching the YAGNI stance of Phase 5/6.
enum class GameEventType
{
	EnemyKilled,
	TriggerFired,
	LoadSceneRequested,
};

struct GameEvent
{
	GameEventType type;
	std::string a;
	std::string b;
};
