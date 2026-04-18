#include "TriggerGameplaySystem.h"
#include "../../Engine/Scene/TriggerComponent.h"
#include "GameplaySceneIds.h"
#include <cstdio>
#include <string>

// OutputDebugStringA for the VS Output window.
// BFWin.h sets up the Windows SDK target-architecture macros and strips
// unused subsystems before pulling in Windows.h — the same pattern used
// by the rest of the project.
#ifdef _WIN32
#include "../../Core/BFWin.h"
#endif

void TriggerGameplaySystem::Update(Scene& scene) noexcept
{
	const SceneObject* player = scene.FindObject(GameplaySceneIds::Player);
	if (player == nullptr)
	{
		return;
	}

	const float px = player->transform.position.x;
	const float pz = player->transform.position.z;

	for (SceneObject& obj : scene.GetObjects())
	{
		if (!obj.triggerComponent.has_value())
		{
			continue;
		}

		TriggerComponent& tc = obj.triggerComponent.value();

		if (tc.fireOnce && tc.fired)
		{
			continue;
		}

		const float cx = obj.transform.position.x;
		const float cz = obj.transform.position.z;
		const float hx = tc.halfExtents.x;
		const float hz = tc.halfExtents.y; // XMFLOAT2: x=half-X, y=half-Z

		const bool inside =
			(px >= cx - hx) && (px <= cx + hx) &&
			(pz >= cz - hz) && (pz <= cz + hz);

		if (inside)
		{
			tc.fired = true;

			const std::string msg =
				"[Trigger] Player entered '" + tc.tag +
				"' (object='" + obj.name + "')\n";

			std::fputs(msg.c_str(), stdout);

#ifdef _WIN32
			::OutputDebugStringA(msg.c_str());
#endif
		}
	}
}
