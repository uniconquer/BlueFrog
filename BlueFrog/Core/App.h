#pragma once
#include "Window.h"
#include "BFTimer.h"
#include "Renderer.h"
#include "../Engine/Camera/TopDownCamera.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/UI/HudState.h"
#include "../Engine/UI/UIRenderer.h"
#include "../Engine/UI/TextRenderer.h"
#include "../Engine/Render/DebugRenderer.h"
#include "../Engine/Render/WorldGridRenderer.h"
#include "../Engine/Audio/AudioEngine.h"
#include "../Game/Simulation/GameplaySimulation.h"
#include <string>

class App
{
public:
	explicit App(std::string scenePath = {});
	int Go();
private:
	void DoFrame(float dt);
	void UpdateModel(const GameplayInput& input, float dt) noexcept;
	GameplayInput CollectGameplayInput(float dt) noexcept;
	void PollDebugToggles() noexcept;
	void ComposeFrame();
private:
	Window wnd;
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
	BFTimer timer;
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
};
