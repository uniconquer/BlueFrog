#pragma once
#include "Window.h"
#include "BFTimer.h"
#include "Renderer.h"
#include "../Engine/Camera/TopDownCamera.h"
#include "../Engine/Scene/Scene.h"
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
	void BuildTestScene();
	void UpdateScene() noexcept;
	void ComposeFrame();
private:
	Window wnd;
	Renderer renderer;
	TopDownCamera camera;
	Scene scene;
	SimpleEnemyController enemyController;
	PlayerController playerController;
	BFTimer timer;
	float simulationTime = 0.0f;
	float lastFrameTime = 0.0f;
};
