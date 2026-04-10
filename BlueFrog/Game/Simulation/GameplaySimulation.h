#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"
#include "GameplayInput.h"
#include "GameplayArenaBuilder.h"
#include "../Hud/HudPresenter.h"
#include "../NPC/SimpleEnemyController.h"
#include "../Player/PlayerController.h"
#include <string>

class GameplaySimulation final
{
public:
	static void BuildArena(Scene& scene, TopDownCamera& camera) noexcept;
	[[nodiscard]] HudState Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept;
	[[nodiscard]] HudState BuildHudState(const Scene& scene) const noexcept;
	[[nodiscard]] static std::wstring BuildWindowTitle(const HudState& hudState) noexcept;
private:
	static void ApplyCameraInput(const GameplayInput& input, TopDownCamera& camera) noexcept;
	PlayerController playerController;
	SimpleEnemyController enemyController;
};
