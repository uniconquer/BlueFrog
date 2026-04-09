#include "GameplaySimulation.h"

HudState GameplaySimulation::Update(Window& wnd, Scene& scene, TopDownCamera& camera, float dt, bool attackQueued) noexcept
{
	playerController.Update(wnd, scene, camera, dt, attackQueued);
	enemyController.Update(scene, dt);
	return BuildHudState(scene);
}

HudState GameplaySimulation::BuildHudState(const Scene& scene) const noexcept
{
	return HudPresenter::Build(scene, playerController);
}
