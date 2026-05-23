#include "InteractionSystem.h"

#include "../Simulation/GameplaySceneIds.h"

#include <cmath>

namespace
{
	// Cheap ASCII narrow→wide for HUD display. NPC names are validator-
	// constrained ASCII at this point so widening 1:1 is correct.
	std::wstring Widen(const std::string& s)
	{
		std::wstring out;
		out.reserve(s.size());
		for (char c : s)
		{
			out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
		}
		return out;
	}
}

namespace InteractionSystem
{
	const SceneObject* Tick(const Scene& scene, HudState& hud, bool suppressPrompt) noexcept
	{
		hud.hasInteractPrompt = false;
		hud.interactPromptName.clear();

		const SceneObject* player = scene.FindObject(GameplaySceneIds::Player);
		if (player == nullptr)
		{
			return nullptr;
		}

		const SceneObject* best = nullptr;
		float bestDistSq = InteractRange * InteractRange;
		for (const SceneObject& obj : scene.GetObjects())
		{
			if (&obj == player) continue;
			if (!obj.npcComponent.has_value()) continue;

			const float dx = obj.transform.position.x - player->transform.position.x;
			const float dz = obj.transform.position.z - player->transform.position.z;
			const float distSq = dx * dx + dz * dz;
			if (distSq < bestDistSq)
			{
				best = &obj;
				bestDistSq = distSq;
			}
		}

		if (best == nullptr || suppressPrompt)
		{
			return best;
		}

		// Prefer the explicit displayName; fall back to the SceneObject
		// name so a villager prefab without an override still produces a
		// readable prompt.
		const std::string& label = best->npcComponent->displayName.empty()
			? best->name
			: best->npcComponent->displayName;
		hud.interactPromptName = Widen(label);
		hud.hasInteractPrompt  = !hud.interactPromptName.empty();
		return best;
	}
}
