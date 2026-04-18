#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"
#include "../Objectives/ObjectiveSystem.h"
#include "GameplayCameraSystem.h"
#include "EnemyGameplaySystem.h"
#include "GameplayInput.h"
#include "GameplayArenaBuilder.h"
#include "PlayerGameplaySystem.h"
#include "TriggerGameplaySystem.h"
#include <string>

class GameplaySimulation final
{
public:
	// Loads the scene at `scenePath` into `scene`/`camera` and resets the
	// objective state from the scene JSON's "objective" block (empty if none).
	// Non-static: owns the ObjectiveSystem whose state must also reset.
	void BuildArena(Scene& scene, TopDownCamera& camera, const std::string& scenePath) noexcept;

	[[nodiscard]] HudState Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept;
	[[nodiscard]] HudState BuildHudState(const Scene& scene) const noexcept;
	[[nodiscard]] static std::wstring BuildWindowTitle(const HudState& hudState) noexcept;
private:
	GameplayCameraSystem  cameraSystem;
	PlayerGameplaySystem  playerSystem;
	EnemyGameplaySystem   enemySystem;
	TriggerGameplaySystem triggerSystem;
	ObjectiveSystem       objectiveSystem;
	EventBus              eventBus;
};
