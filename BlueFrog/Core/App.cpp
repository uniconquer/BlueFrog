#include "App.h"
#include <sstream>
#include <iomanip>

App::App()
	:
	wnd(800, 600, L"Blue Frog"),
	renderer(wnd.Gfx()),
	camera(static_cast<float>(wnd.GetWidth()) / static_cast<float>(wnd.GetHeight()))
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
	HandleCameraInput(dt);
}

void App::HandleCameraInput(float dt) noexcept
{
	const float panSpeed = 7.5f * dt;
	const float orbitSpeed = 1.2f * dt;

	if (wnd.kbd.KeyIsPressed(VK_LEFT))
	{
		camera.MoveTarget(-panSpeed, 0.0f);
	}
	if (wnd.kbd.KeyIsPressed(VK_RIGHT))
	{
		camera.MoveTarget(panSpeed, 0.0f);
	}
	if (wnd.kbd.KeyIsPressed(VK_UP))
	{
		camera.MoveTarget(0.0f, panSpeed);
	}
	if (wnd.kbd.KeyIsPressed(VK_DOWN))
	{
		camera.MoveTarget(0.0f, -panSpeed);
	}
	if (wnd.kbd.KeyIsPressed('Q'))
	{
		camera.RotateAroundTarget(-orbitSpeed);
	}
	if (wnd.kbd.KeyIsPressed('E'))
	{
		camera.RotateAroundTarget(orbitSpeed);
	}

	while (const auto event = wnd.mouse.Read())
	{
		switch (event->GetType())
		{
		case Mouse::Event::Type::WheelUp:
			camera.AdjustZoom(-1.0f);
			break;
		case Mouse::Event::Type::WheelDown:
			camera.AdjustZoom(1.0f);
			break;
		default:
			break;
		}
	}
}

void App::ComposeFrame()
{
	std::wostringstream oss;
	oss << L"Time elapsed : " << std::setprecision(1) << std::fixed << simulationTime
		<< L"s | dt : " << std::setprecision(2) << std::fixed << (lastFrameTime * 1000.0f) << L"ms"
		<< L" | Arrow: pan | Q/E: orbit | Wheel: zoom";
	wnd.SetTitle(oss.str());

	const float blue = sin(simulationTime * 0.7f) * 0.15f + 0.25f;
	wnd.Gfx().BeginFrame(0.05f, 0.08f, blue);
	renderer.DrawTestScene(camera, simulationTime);
	wnd.Gfx().EndFrame();
}
