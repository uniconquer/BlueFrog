#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"
#include "GameplayCameraSystem.h"
#include "EnemyGameplaySystem.h"
#include "GameplayInput.h"
#include "GameplayArenaBuilder.h"
#include "PlayerGameplaySystem.h"
#include <string>

class GameplaySimulation final
{
public:
	static void BuildArena(Scene& scene, TopDownCamera& camera, const std::string& scenePath) noexcept;
	[[nodiscard]] HudState Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept;
	[[nodiscard]] HudState BuildHudState(const Scene& scene) const noexcept;
	[[nodiscard]] static std::wstring BuildWindowTitle(const HudState& hudState) noexcept;
private:
	GameplayCameraSystem cameraSystem;
	PlayerGameplaySystem playerSystem;
	EnemyGameplaySystem enemySystem;
};
