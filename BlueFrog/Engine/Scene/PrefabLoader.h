#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

// Minimal prefab loader with shallow per-component override semantics.
// A prefab JSON file contains a single top-level object — the same shape as one
// entry in a scene's "objects" array, but without "name" (scenes own the name).
// Only the scene's immediate top-level keys are merged against the prefab:
// if the scene already declares a component (e.g. "transform"), it wins
// wholesale; missing components are copied from the prefab.
// Prefab-referencing-prefab is intentionally NOT supported in Phase 5.
class PrefabLoader
{
public:
	using Cache = std::unordered_map<std::string, nlohmann::json>;

	// Loads (or retrieves from `cache`) the prefab at `prefabPath`, then merges
	// its top-level keys into `targetObj` for any key not already present.
	// Returns false on I/O or parse failure and writes a human-readable reason
	// into `errorOut` (may be null).
	static bool LoadAndMerge(const std::filesystem::path& prefabPath,
	                         nlohmann::json& targetObj,
	                         Cache& cache,
	                         std::string* errorOut);

	// Dry-run parse for startup validation: verifies the file opens, parses,
	// and the root is a JSON object. Error messages are prefixed with the
	// prefab file path so a failing sweep can identify the offender.
	static bool Validate(const std::filesystem::path& prefabPath,
	                     std::string* errorOut);
};
