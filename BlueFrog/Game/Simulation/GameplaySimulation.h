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
	[[nodiscard]] HudState Update(Window& wnd, Scene& scene, TopDownCamera& camera, float dt, bool attackQueued) noexcept;
	[[nodiscard]] HudState BuildHudState(const Scene& scene) const noexcept;
private:
	PlayerController playerController;
	SimpleEnemyController enemyController;
};
