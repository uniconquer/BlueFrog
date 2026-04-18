#include "GameplaySimulation.h"
#include <sstream>
#include <string>
#include <utility>

void GameplaySimulation::BuildArena(Scene& scene, TopDownCamera& camera, const std::string& scenePath) noexcept
{
	// Scene JSON's "objective" block (if any) is parsed into `state`; absent
	// block yields a default-constructed state so the system resets cleanly
	// across mid-play reloads.
	ObjectiveState state;
	GameplayArenaBuilder::Build(scene, camera, scenePath, state);
	objectiveSystem.Reset(std::move(state));
}

void GameplaySimulation::ReloadScene(const std::string& scenePath, Scene& scene, TopDownCamera& camera) noexcept
{
	// A reload is semantically a full reset: Scene::Clear (inside Load) wipes
	// all objects including trigger fired-flags, ObjectiveSystem::Reset
	// discards objective progress, and pendingSceneLoad is cleared so we
	// don't trigger a second reload next tick from a stale request.
	BuildArena(scene, camera, scenePath);
	pendingSceneLoad.reset();
}

std::optional<std::string> GameplaySimulation::ConsumePendingSceneLoad() noexcept
{
	std::optional<std::string> out;
	out.swap(pendingSceneLoad);
	return out;
}

HudState GameplaySimulation::Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept
{
	cameraSystem.ApplyInput(input, camera);
	playerSystem.Update(input, scene, camera, dt, eventBus);
	enemySystem.Update(scene, dt, eventBus);
	triggerSystem.Update(scene, eventBus);
	cameraSystem.FollowPlayer(scene, camera);

	// Drain exactly once per tick after all systems have run. Scene-load
	// requests are captured first so they latch even if the drain occurs in
	// the same tick as an EnemyKilled event; ObjectiveSystem still sees the
	// full event batch so a scripted "kill and transition" pair won't drop
	// kill credit on the outgoing scene.
	auto events = eventBus.Drain();
	for (const auto& e : events)
	{
		if (e.type == GameEventType::LoadSceneRequested && !e.a.empty())
		{
			// Last-write-wins if multiple requests queue in a single tick.
			// In practice trigger fireOnce prevents this, but explicit
			// behaviour beats "first request by accident".
			pendingSceneLoad = e.a;
		}
	}
	objectiveSystem.Consume(events);

	HudState hud = BuildHudState(scene);
	hud.objectiveText = objectiveSystem.CurrentText();
	return hud;
}

HudState GameplaySimulation::BuildHudState(const Scene& scene) const noexcept
{
	HudState hud = playerSystem.BuildHudState(scene);
	hud.objectiveText = objectiveSystem.CurrentText();
	return hud;
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
