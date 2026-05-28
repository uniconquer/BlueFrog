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
			// Two kinds of interactables share the same "approachable
			// thing" scan: NPCs (Talk to) and Mounts (Mount). Both flow
			// through the same E-key handler; the verb difference lives
			// only in the prompt label + the action FLApp picks.
			const bool hasNpc   = obj.npcComponent.has_value();
			const bool hasMount = obj.mountComponent.has_value();
			if (!hasNpc && !hasMount) continue;
			// Mounts that already have a rider should not advertise
			// themselves — re-mount prompt while already mounted reads
			// as a bug.
			if (hasMount && obj.mountComponent->occupied) continue;

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

		// Verb + label depend on which interactable kind we found. Mount
		// wins if both are present on the same object (shouldn't happen
		// at v1 but keeps the policy deterministic).
		std::string verb;
		std::string label;
		if (best->mountComponent.has_value())
		{
			verb  = "Mount";
			label = best->mountComponent->displayName.empty()
				? best->name
				: best->mountComponent->displayName;
		}
		else
		{
			verb  = "Talk to";
			label = best->npcComponent->displayName.empty()
				? best->name
				: best->npcComponent->displayName;
		}
		hud.interactPromptName = Widen(label);
		hud.interactPromptVerb = Widen(verb);
		hud.hasInteractPrompt  = !hud.interactPromptName.empty();
		return best;
	}
}
