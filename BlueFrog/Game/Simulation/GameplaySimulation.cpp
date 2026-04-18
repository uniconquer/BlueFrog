#include "GameplaySimulation.h"
#include <sstream>
#include <string>

#ifdef _WIN32
#include "../../Core/BFWin.h"
#endif

void GameplaySimulation::BuildArena(Scene& scene, TopDownCamera& camera, const std::string& scenePath) noexcept
{
	GameplayArenaBuilder::Build(scene, camera, scenePath);
}

namespace
{
	// Phase 6 A-2: temporary debug drain. Proves that EnemyKilled and
	// TriggerFired publish at the expected moments. A-3 replaces this with
	// ObjectiveSystem consumption.
	void DebugDumpEvents(const std::vector<GameEvent>& events)
	{
#ifndef _WIN32
		(void)events;
#else
		for (const auto& e : events)
		{
			const char* name = "Unknown";
			switch (e.type)
			{
			case GameEventType::EnemyKilled:        name = "EnemyKilled"; break;
			case GameEventType::TriggerFired:       name = "TriggerFired"; break;
			case GameEventType::LoadSceneRequested: name = "LoadSceneRequested"; break;
			}
			const std::string msg = std::string("[Event] ") + name + " a='" + e.a + "' b='" + e.b + "'\n";
			::OutputDebugStringA(msg.c_str());
		}
#endif
	}
}

HudState GameplaySimulation::Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept
{
	cameraSystem.ApplyInput(input, camera);
	playerSystem.Update(input, scene, camera, dt, eventBus);
	enemySystem.Update(scene, dt, eventBus);
	triggerSystem.Update(scene, eventBus);
	cameraSystem.FollowPlayer(scene, camera);

	// Drain exactly once per tick after all systems have run. A-3 replaces
	// this debug dump with ObjectiveSystem::Consume.
	DebugDumpEvents(eventBus.Drain());

	return BuildHudState(scene);
}

HudState GameplaySimulation::BuildHudState(const Scene& scene) const noexcept
{
	return playerSystem.BuildHudState(scene);
}

std::wstring GameplaySimulation::BuildWindowTitle(const HudState& hudState) noexcept
{
	std::wostringstream oss;
	oss << L"Blue Frog | HP " << static_cast<int>(hudState.playerHealth.current) << L"/" << static_cast<int>(hudState.playerHealth.max);
	if (hudState.hasTarget)
	{
		oss << L" | Enemy " << static_cast<int>(hudState.targetHealth.current) << L"/" << static_cast<int>(hudState.targetHealth.max);
	}
	oss << L" | " << hudState.objectiveText << L" | Q/E: orbit | Wheel: zoom";
	return oss.str();
}
