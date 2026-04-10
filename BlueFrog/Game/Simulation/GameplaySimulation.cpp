#include "GameplaySimulation.h"
#include <sstream>

void GameplaySimulation::ApplyCameraInput(const GameplayInput& input, TopDownCamera& camera) noexcept
{
	if (input.orbitDelta != 0.0f)
	{
		camera.RotateAroundTarget(input.orbitDelta);
	}

	if (input.zoomDelta != 0.0f)
	{
		camera.AdjustZoom(input.zoomDelta);
	}
}

void GameplaySimulation::BuildArena(Scene& scene, TopDownCamera& camera) noexcept
{
	GameplayArenaBuilder::Build(scene, camera);
}

HudState GameplaySimulation::Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept
{
	ApplyCameraInput(input, camera);
	playerController.Update(input, scene, camera, dt);
	enemyController.Update(scene, dt);
	return BuildHudState(scene);
}

HudState GameplaySimulation::BuildHudState(const Scene& scene) const noexcept
{
	return HudPresenter::Build(scene, playerController);
}

std::wstring GameplaySimulation::BuildWindowTitle(const HudState& hudState) noexcept
{
	std::wostringstream oss;
	oss << L"Blue Frog | HP " << static_cast<int>(hudState.playerHealth.current)
		<< L"/" << static_cast<int>(hudState.playerHealth.max);

	if (hudState.hasTarget)
	{
		oss << L" | Enemy " << static_cast<int>(hudState.targetHealth.current)
			<< L"/" << static_cast<int>(hudState.targetHealth.max);
	}

	oss << L" | " << hudState.objectiveText << L" | Q/E: orbit | Wheel: zoom";
	return oss.str();
}
