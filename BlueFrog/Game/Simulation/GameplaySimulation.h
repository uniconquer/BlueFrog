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
#include <optional>
#include <string>

class GameplaySimulation final
{
public:
	// Loads the scene at `scenePath` into `scene`/`camera` and resets the
	// objective state from the scene JSON's "objective" block (empty if none).
	// Non-static: owns the ObjectiveSystem whose state must also reset.
	void BuildArena(Scene& scene, TopDownCamera& camera, const std::string& scenePath) noexcept;

	// Unified reload entry point. Equivalent to BuildArena today, but owns
	// the contract that a scene transition clears *all* gameplay state
	// (scene graph, objective progress, trigger fired flags — everything
	// Scene::Clear and ObjectiveSystem::Reset reach between them). Callers
	// go through this so future additions (player carry-over, save state)
	// have a single integration point.
	void ReloadScene(const std::string& scenePath, Scene& scene, TopDownCamera& camera) noexcept;

	// Drains the queued scene-load request (set when a LoadSceneRequested
	// event is consumed during Update). Returns nullopt when no reload is
	// pending. App processes this *after* UpdateModel to avoid mutating the
	// scene while systems still hold live references into it.
	[[nodiscard]] std::optional<std::string> ConsumePendingSceneLoad() noexcept;

	[[nodiscard]] HudState Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept;
	[[nodiscard]] HudState BuildHudState(const Scene& scene) const noexcept;
	[[nodiscard]] static std::wstring BuildWindowTitle(const HudState& hudState) noexcept;
private:
	GameplayCameraSystem         cameraSystem;
	PlayerGameplaySystem         playerSystem;
	EnemyGameplaySystem          enemySystem;
	TriggerGameplaySystem        triggerSystem;
	ObjectiveSystem              objectiveSystem;
	EventBus                     eventBus;
	std::optional<std::string>   pendingSceneLoad;
};
