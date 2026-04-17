#include "PrefabLoader.h"
#include <fstream>

bool PrefabLoader::LoadAndMerge(const std::filesystem::path& prefabPath,
                                nlohmann::json& targetObj,
                                Cache& cache,
                                std::string* errorOut)
{
	const std::string key = prefabPath.string();

	auto it = cache.find(key);
	if (it == cache.end())
	{
		std::ifstream file(prefabPath);
		if (!file.is_open())
		{
			if (errorOut) *errorOut = "PrefabLoader: cannot open prefab file: " + key;
			return false;
		}

		nlohmann::json parsed;
		try
		{
			parsed = nlohmann::json::parse(file);
		}
		catch (const nlohmann::json::parse_error& e)
		{
			if (errorOut) *errorOut = "PrefabLoader: parse error in '" + key + "': " + e.what();
			return false;
		}

		if (!parsed.is_object())
		{
			if (errorOut) *errorOut = "PrefabLoader: '" + key + "' root must be a JSON object";
			return false;
		}

		it = cache.emplace(key, std::move(parsed)).first;
	}

	const nlohmann::json& prefab = it->second;
	for (auto field = prefab.begin(); field != prefab.end(); ++field)
	{
		const auto& fieldKey = field.key();
		// Scene object's "name" is authoritative; prefab name (if any) is ignored.
		if (fieldKey == "name") continue;
		// Prefabs referencing prefabs are not supported; silently ignore nested refs.
		if (fieldKey == "prefab") continue;
		// Shallow override: scene-side declaration wins if present.
		if (targetObj.contains(fieldKey)) continue;
		targetObj[fieldKey] = field.value();
	}

	return true;
}
