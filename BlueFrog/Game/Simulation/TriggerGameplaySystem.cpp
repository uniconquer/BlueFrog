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

void TriggerGameplaySystem::Update(const SystemContext& ctx) noexcept
{
	Scene& scene = ctx.scene;
	EventBus& bus = ctx.eventBus;
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

			// Action-type resolution. Absent action => implicit "log" so
			// Phase 5 scenes still behave identically.
			const std::string& actionType = tc.action.has_value()
				? tc.action->type
				: std::string("log");
			const std::string& actionParam = tc.action.has_value()
				? tc.action->param
				: std::string();

			if (actionType == "log")
			{
				const std::string msg =
					"[Trigger] Player entered '" + tc.tag +
					"' (object='" + obj.name + "')\n";
				std::fputs(msg.c_str(), stdout);
#ifdef _WIN32
				::OutputDebugStringA(msg.c_str());
#endif
			}
			else if (actionType == "publish")
			{
				// Still log so debugging parity with "log" holds, then emit
				// the event. Payload b carries the action param rather than
				// the object name — `publish` is the action that gives the
				// scene author full control over what downstream sees.
				const std::string msg =
					"[Trigger] Player entered '" + tc.tag +
					"' (object='" + obj.name + "') -> publish param='" + actionParam + "'\n";
				std::fputs(msg.c_str(), stdout);
#ifdef _WIN32
				::OutputDebugStringA(msg.c_str());
#endif
				bus.Publish({ GameEventType::TriggerFired, tc.tag, actionParam });
				continue;
			}
			else if (actionType == "load_scene")
			{
				// Log only once (this is a one-shot by design — the scene
				// about to load will wipe fired state anyway). Actual
				// reload is handled by GameplaySimulation after Update
				// returns, so no iterators here get invalidated.
				const std::string msg =
					"[Trigger] Player entered '" + tc.tag +
					"' (object='" + obj.name + "') -> load_scene '" + actionParam + "'\n";
				std::fputs(msg.c_str(), stdout);
#ifdef _WIN32
				::OutputDebugStringA(msg.c_str());
#endif
				bus.Publish({ GameEventType::LoadSceneRequested, actionParam, {} });
				continue;
			}

			// Default path (log-only actions) still publishes TriggerFired so
			// other consumers that care about "player entered tag X" keep
			// working without the scene needing to opt into "publish".
			// Payload: a = tag (scene-defined intent), b = object name
			// (useful when multiple trigger zones share a tag).
			bus.Publish({ GameEventType::TriggerFired, tc.tag, obj.name });
		}
	}
}
