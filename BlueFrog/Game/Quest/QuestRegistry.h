#pragma once

#include "Quest.h"

#include <filesystem>
#include <string>
#include <unordered_map>

// Holds the loaded definitions of every quest the game knows about.
// Populated once at boot by LoadAll(); never mutated afterward. The
// runtime per-NPC quest *state* (Available/Active/Complete/TurnedIn,
// per-condition progress) lives in QuestSystem — this registry only
// owns the static definitions.
//
// Quest files live in BlueFrog/Assets/Quests/*.quest.json and are
// scanned at boot the same way SceneLoader / PrefabLoader sweep their
// directories.
class QuestRegistry
{
public:
	QuestRegistry() = default;
	QuestRegistry(const QuestRegistry&) = delete;
	QuestRegistry& operator=(const QuestRegistry&) = delete;

	// Sweeps `directory` for `*.quest.json` files and registers each
	// successfully-parsed Quest by its `id`. Returns false (with
	// `errorOut` populated) on the first parse failure, mirroring the
	// boot-validator style. A missing directory is treated as empty.
	bool LoadAll(const std::filesystem::path& directory, std::string* errorOut);

	// Returns the loaded quest by id, or nullptr when not registered.
	// Pointer is stable for the lifetime of the registry — no mutation
	// happens after LoadAll.
	[[nodiscard]] const Quest* Find(const std::string& id) const noexcept;

	[[nodiscard]] std::size_t Size() const noexcept { return quests_.size(); }

private:
	std::unordered_map<std::string, Quest> quests_;
};
