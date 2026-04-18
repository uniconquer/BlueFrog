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

HudState GameplaySimulation::Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept
{
	cameraSystem.ApplyInput(input, camera);
	playerSystem.Update(input, scene, camera, dt, eventBus);
	enemySystem.Update(scene, dt, eventBus);
	triggerSystem.Update(scene, eventBus);
	cameraSystem.FollowPlayer(scene, camera);

	// Drain exactly once per tick after all systems have run. Feeding the
	// full batch to ObjectiveSystem keeps condition matching deterministic
	// with respect to publish order (A-4 will also scan for scene-load
	// requests before consuming).
	auto events = eventBus.Drain();
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
