#pragma once
#include "Window.h"
#include "BFTimer.h"
#include "Renderer.h"
#include "../Engine/Camera/TopDownCamera.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/UI/HudState.h"
#include "../Engine/UI/UIRenderer.h"
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
	void ComposeFrame();
private:
	Window wnd;
	Renderer renderer;
	UIRenderer uiRenderer;
	TopDownCamera camera;
	Scene scene;
	HudState hudState;
	GameplaySimulation gameplaySimulation;
	BFTimer timer;
};
