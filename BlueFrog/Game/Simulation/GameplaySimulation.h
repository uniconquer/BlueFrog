#pragma once

#include "../../Core/Window.h"
#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"
#include "../Hud/HudPresenter.h"
#include "../NPC/SimpleEnemyController.h"
#include "../Player/PlayerController.h"

class GameplaySimulation final
{
public:
	void Update(Window& wnd, Scene& scene, TopDownCamera& camera, float dt, bool attackQueued) noexcept
	{
		playerController.Update(wnd, scene, camera, dt, attackQueued);
		enemyController.Update(scene, dt);
	}

	[[nodiscard]] HudState BuildHudState(const Scene& scene) const noexcept
	{
		return HudPresenter::Build(scene, playerController);
	}

	[[nodiscard]] PlayerController& Player() noexcept
	{
		return playerController;
	}

	[[nodiscard]] const PlayerController& Player() const noexcept
	{
		return playerController;
	}

private:
	PlayerController playerController;
	SimpleEnemyController enemyController;
};
