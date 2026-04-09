#pragma once
#include "Window.h"
#include "BFTimer.h"
#include "Renderer.h"
#include "../Engine/Camera/TopDownCamera.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/UI/HudState.h"
#include "../Engine/UI/UIRenderer.h"
#include "../Game/NPC/SimpleEnemyController.h"
#include "../Game/Player/PlayerController.h"

class App
{
public:
	App();
	int Go();
private:
	void DoFrame(float dt);
	void UpdateModel(float dt) noexcept;
	bool HandleCameraInput(float dt) noexcept;
	void BuildArenaScene();
	void UpdateHudState() noexcept;
	void ComposeFrame();
private:
	Window wnd;
	Renderer renderer;
	UIRenderer uiRenderer;
	TopDownCamera camera;
	Scene scene;
	HudState hudState;
	SimpleEnemyController enemyController;
	PlayerController playerController;
	BFTimer timer;
};
