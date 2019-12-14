#pragma once
#include "Window.h"
#include "BFTimer.h"

class App
{
public:
	App();
	int Go();
private:
	void DoFrame();
private:
	Window wnd;
	BFTimer timer;
};