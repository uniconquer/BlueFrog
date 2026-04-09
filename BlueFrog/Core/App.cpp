#include "App.h"
#include <sstream>
#include <iomanip>

App::App()
	:
	wnd(800, 600, L"Blue Frog"),
	renderer(wnd.Gfx()),
	camera(static_cast<float>(wnd.GetWidth()) / static_cast<float>(wnd.GetHeight()))
{
	BuildTestScene();
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
	UpdateScene();
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

void App::BuildTestScene()
{
	scene.Clear();

	auto& ground = scene.CreateObject("Ground");
	ground.transform.scale = { 18.0f, 1.0f, 18.0f };
	ground.renderComponent = RenderComponent{ RenderMeshType::Plane };

	auto& central = scene.CreateObject("Central");
	central.transform.position = { 0.0f, 1.25f, 0.0f };
	central.transform.scale = { 1.35f, 1.35f, 1.35f };
	central.renderComponent = RenderComponent{ RenderMeshType::Cube };

	auto& northWall = scene.CreateObject("NorthWall");
	northWall.transform.position = { 0.0f, 1.0f, 6.5f };
	northWall.transform.scale = { 6.0f, 1.0f, 0.7f };
	northWall.renderComponent = RenderComponent{ RenderMeshType::Cube };

	auto& southWall = scene.CreateObject("SouthWall");
	southWall.transform.position = { 0.0f, 1.0f, -6.5f };
	southWall.transform.scale = { 6.0f, 1.0f, 0.7f };
	southWall.renderComponent = RenderComponent{ RenderMeshType::Cube };

	auto& eastWall = scene.CreateObject("EastWall");
	eastWall.transform.position = { 6.5f, 1.0f, 0.0f };
	eastWall.transform.scale = { 0.7f, 1.0f, 6.0f };
	eastWall.renderComponent = RenderComponent{ RenderMeshType::Cube };

	auto& westWall = scene.CreateObject("WestWall");
	westWall.transform.position = { -6.5f, 1.0f, 0.0f };
	westWall.transform.scale = { 0.7f, 1.0f, 6.0f };
	westWall.renderComponent = RenderComponent{ RenderMeshType::Cube };

	auto& pillarA = scene.CreateObject("PillarA");
	pillarA.transform.position = { -3.5f, 0.8f, -2.5f };
	pillarA.transform.scale = { 0.8f, 0.8f, 0.8f };
	pillarA.renderComponent = RenderComponent{ RenderMeshType::Cube };

	auto& pillarB = scene.CreateObject("PillarB");
	pillarB.transform.position = { 3.5f, 0.8f, 2.5f };
	pillarB.transform.scale = { 0.8f, 0.8f, 0.8f };
	pillarB.renderComponent = RenderComponent{ RenderMeshType::Cube };
}

void App::UpdateScene() noexcept
{
	if (auto* central = scene.FindObject("Central"))
	{
		central->transform.rotation = { simulationTime * 0.15f, simulationTime, simulationTime * 0.08f };
	}

	if (auto* pillarA = scene.FindObject("PillarA"))
	{
		pillarA->transform.rotation = { 0.0f, simulationTime * 0.5f, 0.0f };
	}

	if (auto* pillarB = scene.FindObject("PillarB"))
	{
		pillarB->transform.rotation = { 0.0f, -simulationTime * 0.45f, 0.0f };
	}
}

void App::ComposeFrame()
{
	std::wostringstream oss;
	oss << L"Time elapsed : " << std::setprecision(1) << std::fixed << simulationTime
		<< L"s | dt : " << std::setprecision(2) << std::fixed << (lastFrameTime * 1000.0f) << L"ms"
		<< L" | Scene objects: " << scene.GetObjects().size()
		<< L" | Arrow: pan | Q/E: orbit | Wheel: zoom";
	wnd.SetTitle(oss.str());

	const float blue = sin(simulationTime * 0.7f) * 0.15f + 0.25f;
	wnd.Gfx().BeginFrame(0.05f, 0.08f, blue);
	renderer.Render(scene, camera);
	wnd.Gfx().EndFrame();
}
