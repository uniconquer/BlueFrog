#include "GameplaySimulation.h"
#include <sstream>

void GameplaySimulation::BuildArena(Scene& scene, TopDownCamera& camera) noexcept
{
	GameplayArenaBuilder::Build(scene, camera);
}

HudState GameplaySimulation::Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept
{
	cameraSystem.ApplyInput(input, camera);
	playerSystem.Update(input, scene, camera, dt);
	enemySystem.Update(scene, dt);
	cameraSystem.FollowPlayer(scene, camera);
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
