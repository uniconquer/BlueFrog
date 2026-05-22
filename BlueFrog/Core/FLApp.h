#pragma once

#include "AppBase.h"

#include "Renderer.h"
#include "../Engine/Camera/TopDownCamera.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/UI/DamagePopup.h"
#include "../Engine/UI/HudState.h"
#include "../Engine/UI/UIRenderer.h"
#include "../Engine/UI/TextRenderer.h"
#include "../Engine/Render/DebugRenderer.h"
#include "../Engine/Render/WorldGridRenderer.h"
#include "../Engine/Audio/AudioEngine.h"
#include "../Game/Simulation/GameplaySimulation.h"
#include "../Game/Profile/PlayerProfile.h"
#include <string>
#include <vector>

// FL game-side application. Subclasses BlueFrogEngine's AppBase to plug
// the game's renderers, scene, simulation, audio, and HUD into the
// engine's per-frame hooks. AppBase owns the OS window + main loop; this
// class owns everything specific to FL.
//
// Why a subclass instead of composition: matches Unity's MonoBehaviour
// and Unreal's AActor::Tick pattern — game code lives in virtual
// overrides of well-known lifecycle hooks (OnStartup / OnUpdate /
// OnRender / OnShutdown). The next FL-or-other game can swap this whole
// class out and the engine doesn't change.
class FLApp final : public AppBase
{
public:
	explicit FLApp(std::string scenePath = {});

protected:
	void OnStartup() override;
	void OnUpdate(float dt) override;
	void OnRender() override;

private:
	void UpdateModel(const GameplayInput& input, float dt) noexcept;
	GameplayInput CollectGameplayInput(float dt) noexcept;
	void PollDebugToggles() noexcept;

	Renderer renderer;
	UIRenderer uiRenderer;
	TextRenderer textRenderer;
	DebugRenderer debugRenderer;
	WorldGridRenderer worldGridRenderer;
	TopDownCamera camera;
	Scene scene;
	HudState hudState;
	GameplaySimulation gameplaySimulation;
	AudioEngine audio;
	std::string currentScenePath;
	bool   debugGizmosEnabled  = false;
	bool   worldGridEnabled    = false;
	bool   reloadRequested     = false;
	bool   inspectorEnabled    = false;
	int    inspectorSelected   = 0;
	int    inspectorFieldIndex = 0;
	// Damage feedback: when player HP drops between ticks we light up a
	// transient fullscreen red overlay. lastPlayerHealth tracks the value
	// from the previous tick (-1 = uninitialized); damageFlashAlpha
	// linearly fades to 0 over kDamageFlashDuration after each hit.
	int    lastPlayerHealth   = -1;
	float  damageFlashAlpha   = 0.0f;
	// Persistent profile state (Phase H Stage 1). currentPlayTimeSec
	// accumulates monotonically across the session; F8 snapshots it
	// alongside scene + HP into the save file. The Save folder lives
	// outside the build outputs so a clean rebuild doesn't wipe progress.
	float  currentPlayTimeSec = 0.0f;
	// Live floating damage-number popups. Combat code appends new entries
	// via the SystemContext sink; FLApp ticks ages each frame and erases
	// expired ones before TextRenderer projects the remainder. Owned here
	// (rather than on GameplaySimulation) so a scene transition does not
	// drop in-flight popups — though in practice ReloadScene also resets
	// HP and downstream feel, so a clear-on-reload is fine too.
	std::vector<DamagePopup> activePopups;
	// Hit-impact screen shake. shakeMagnitude is in world units (XZ plane
	// translational offset on the camera). OnUpdate kicks it on detected
	// hits — player took damage = heavy kick, popup count grew while HP
	// held steady = the player landed one (lighter kick). shakeTimer
	// advances each frame; the actual per-frame offset is sin(timer*freq)
	// * magnitude * sign in a randomly-rotated direction set at kick time.
	// lastPopupCount is the comparison baseline for the popup delta.
	float  shakeMagnitude   = 0.0f;
	float  shakeTimer       = 0.0f;
	float  shakeDirX        = 1.0f;
	float  shakeDirZ        = 0.0f;
	size_t lastPopupCount   = 0;
};
