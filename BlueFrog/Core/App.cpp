#include "App.h"
#include <sstream>
#include <iomanip>

App::App()
	:
	wnd(800, 600, L"Blue Frog")
{
}

int App::Go()
{
	while (true)
	{
		if (const auto ecode = Window::ProcessMessages())
		{
			return *ecode;
		}

		DoFrame(timer.Mark());
	}
}

void App::DoFrame(float dt)
{
	UpdateModel(dt);
	ComposeFrame();
}

void App::UpdateModel(float dt) noexcept
{
	lastFrameTime = dt;
	simulationTime += dt;
}

void App::ComposeFrame()
{
	std::wostringstream oss;
	oss << L"Time elapsed : " << std::setprecision(1) << std::fixed << simulationTime
		<< L"s | dt : " << std::setprecision(2) << std::fixed << (lastFrameTime * 1000.0f) << L"ms";
	wnd.SetTitle(oss.str());

	const float blue = sin(simulationTime * 0.7f) * 0.15f + 0.25f;
	wnd.Gfx().BeginFrame(0.05f, 0.08f, blue);
	wnd.Gfx().DrawTestTriangle(simulationTime);
	wnd.Gfx().EndFrame();
}
