#pragma once
#include "Window.h"
#include "BFTimer.h"
#include "Renderer.h"

class App
{
public:
	App();
	int Go();
private:
	void DoFrame(float dt);
	void UpdateModel(float dt) noexcept;
	void ComposeFrame();
private:
	Window wnd;
	Renderer renderer;
	BFTimer timer;
	float simulationTime = 0.0f;
	float lastFrameTime = 0.0f;
};
