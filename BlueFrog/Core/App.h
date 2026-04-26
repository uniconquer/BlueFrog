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
#include "../Game/Simulation/GameplaySimulation.h"
#include <string>

class App
{
public:
	explicit App(std::string scenePath = {});
	int Go();
private:
	void DoFrame(float dt);
	void UpdateModel(float dt) noexcept;
	GameplayInput CollectGameplayInput(float dt) noexcept;
	void PollDebugToggles() noexcept;
	void ComposeFrame();
private:
	Window wnd;
	Renderer renderer;
	UIRenderer uiRenderer;
	TextRenderer textRenderer;
	DebugRenderer debugRenderer;
	TopDownCamera camera;
	Scene scene;
	HudState hudState;
	GameplaySimulation gameplaySimulation;
	BFTimer timer;
	std::string currentScenePath;
	bool debugGizmosEnabled  = false;
	bool reloadRequested     = false;
	bool inspectorEnabled    = false;
	int  inspectorSelected   = 0;
	int  inspectorFieldIndex = 0;
};
