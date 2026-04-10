#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"
#include "EnemyGameplaySystem.h"
#include "GameplayInput.h"
#include "GameplayArenaBuilder.h"
#include "PlayerGameplaySystem.h"
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
	PlayerGameplaySystem playerSystem;
	EnemyGameplaySystem enemySystem;
};
