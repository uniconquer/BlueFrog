#pragma once

#include "Quest.h"

#include <filesystem>
#include <string>

// JSON loader for a single .quest.json file. Errors carry a
// path-prefixed message matching the rest of the loader stack so the
// boot validator can surface bad authoring in the MessageBox before
// the window comes up.
//
// Schema (v1):
// {
//   "id": "clear_arena_trial",
//   "title": "Trial of the Arena",
//   "conditions": [ { "type": "enemy_killed", "name": "EnemyScout" }, ... ],
//   "dialogOffer":    "...",
//   "dialogActive":   "...",
//   "dialogComplete": "...",
//   "dialogTurnedIn": "...",
//   "reward": { "healPlayer": 5, "boostMaxHealth": 1 }
// }
//
// `id` and at least one of the dialog lines are required; everything
// else may default to empty/zero.
namespace QuestLoader
{
	bool Load(const std::filesystem::path& path, Quest& out, std::string* errorOut);
}
