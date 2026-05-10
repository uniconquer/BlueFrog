#pragma once

#include <filesystem>
#include <string>

// Persistent player state. Saved to JSON via PlayerProfile::Save and
// restored at app boot via PlayerProfile::Load. Phase H Stage 1 keeps
// the surface tiny on purpose — adding a field is a one-line struct
// change + matching read/write pair, so we extend in response to real
// gameplay needs (kills, achievements, settings) rather than speculation.
//
// Lifetime model:
//   - On launch, App tries to Load("Save/profile.json"). On success the
//     loaded `scenePath` overrides the CLI / default scene argument and
//     the saved HP overrides the spawn HP after BuildArena.
//   - On F8, App writes the current state (current scene path, player
//     HP, total play time) to disk via Save.
//   - Death-driven scene reload does NOT touch the save file. The save
//     is the player's intentional checkpoint.
struct PlayerProfile
{
	std::string scenePath;          // matches App::currentScenePath
	int         playerHealth   = 0; // 0 = no override (use spawn HP)
	int         playerMaxHealth = 0;
	float       playTimeSec    = 0.0f;
};

namespace PlayerProfileIO
{
	// Loads `path` into `out`. Returns true on success; false on any
	// issue (file missing, malformed JSON, missing fields). On false,
	// `out` contents are unspecified — caller treats as "no save".
	bool Load(const std::filesystem::path& path, PlayerProfile& out) noexcept;

	// Writes `profile` to `path`, creating the parent directory if
	// needed. Returns true on success. Logs failures via OutputDebugString.
	bool Save(const std::filesystem::path& path, const PlayerProfile& profile) noexcept;
}
