#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/DamagePopup.h"
#include "../../Engine/UI/HudState.h"
#include <vector>
#include "../Objectives/ObjectiveSystem.h"
#include "GameplayCameraSystem.h"
#include "EnemyGameplaySystem.h"
#include "GameplayInput.h"
#include "GameplayArenaBuilder.h"
#include "PlayerGameplaySystem.h"
#include "TriggerGameplaySystem.h"
#include <optional>
#include <string>

class AudioEngine;

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

	// Returns true exactly once per death sequence: after the player has
	// remained dead for kDeathReloadDelay seconds. The caller is responsible
	// for performing the actual ReloadScene against its tracked
	// currentScenePath — GameplaySimulation does not know the scene path the
	// app booted with. Subsequent calls return false until the next death.
	[[nodiscard]] bool ConsumePendingDeathReload() noexcept;

	// Optional audio sink to thread into the per-tick SystemContext. App
	// installs this once at boot; pass nullptr to disable audio cleanly.
	void SetAudio(AudioEngine* audio) noexcept { audio_ = audio; }

	// Optional sink for damage-number popups (owned by App so the vector
	// survives across scene reloads). Combat code pushes one entry per
	// damage application; App ticks ages and removes expired popups before
	// the next frame. nullptr disables the feature.
	void SetDamagePopupSink(std::vector<DamagePopup>* popups) noexcept { damagePopupsSink_ = popups; }

	// Dialog gating signal. While dialog is active the HUD interaction
	// prompt is suppressed so we don't paint "[E] Talk to X" on top of
	// the open dialog box. The simulation otherwise continues to run —
	// pausing during dialog is a v2 decision.
	void SetDialogActive(bool active) noexcept { dialogActive_ = active; }

	[[nodiscard]] HudState Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept;
	[[nodiscard]] HudState BuildHudState(const Scene& scene) const noexcept;
	[[nodiscard]] static std::wstring BuildWindowTitle(const HudState& hudState) noexcept;

	// Read-only access to the current ObjectiveState. SceneSerializer reads
	// this to write the objective block back to JSON on save. The exposed
	// reference points at live system state — do not stash it past a
	// ReloadScene call, which swaps the underlying object.
	[[nodiscard]] const ObjectiveState& GetObjectiveState() const noexcept;
private:
	GameplayCameraSystem         cameraSystem;
	PlayerGameplaySystem         playerSystem;
	EnemyGameplaySystem          enemySystem;
	TriggerGameplaySystem        triggerSystem;
	ObjectiveSystem              objectiveSystem;
	EventBus                     eventBus;
	std::optional<std::string>   pendingSceneLoad;
	float                        deathTimer        = 0.0f;
	bool                         deathSequenceActive = false;
	bool                         pendingDeathReload  = false;
	AudioEngine*                 audio_              = nullptr;
	std::vector<DamagePopup>*    damagePopupsSink_   = nullptr;
	bool                         dialogActive_       = false;
};
