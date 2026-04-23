#include "App.h"

namespace
{
	constexpr const char* kDefaultScenePath = "Assets/Scenes/arena_trial.json";
}

App::App(std::string scenePath)
	:
	wnd(800, 600, L"Blue Frog"),
	renderer(wnd.Gfx()),
	uiRenderer(wnd.Gfx()),
	textRenderer(wnd.Gfx()),
	camera(static_cast<float>(wnd.GetWidth()) / static_cast<float>(wnd.GetHeight()))
{
	const std::string resolvedScene = scenePath.empty() ? std::string(kDefaultScenePath) : std::move(scenePath);
	gameplaySimulation.BuildArena(scene, camera, resolvedScene);
	hudState = gameplaySimulation.BuildHudState(scene);
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
	hudState = gameplaySimulation.Update(CollectGameplayInput(dt), scene, camera, dt);

	// Honor LoadSceneRequested events drained during Update. Reload happens
	// here — after every system has finished iterating the scene — so no
	// live references into scene objects are invalidated mid-tick. We
	// immediately re-BuildHudState against the new scene to avoid a
	// 1-frame-stale title bar (same pattern as App's constructor).
	if (auto path = gameplaySimulation.ConsumePendingSceneLoad())
	{
		gameplaySimulation.ReloadScene(*path, scene, camera);
		hudState = gameplaySimulation.BuildHudState(scene);
	}
}

GameplayInput App::CollectGameplayInput(float dt) noexcept
{
	const float orbitSpeed = 1.2f * dt;
	GameplayInput input = {};
	input.viewportSize = { static_cast<float>(wnd.GetWidth()), static_cast<float>(wnd.GetHeight()) };
	input.movementIntent =
	{
		(wnd.kbd.KeyIsPressed('D') ? 1.0f : 0.0f) - (wnd.kbd.KeyIsPressed('A') ? 1.0f : 0.0f),
		(wnd.kbd.KeyIsPressed('W') ? 1.0f : 0.0f) - (wnd.kbd.KeyIsPressed('S') ? 1.0f : 0.0f)
	};

	if (wnd.kbd.KeyIsPressed('Q'))
	{
		input.orbitDelta -= orbitSpeed;
	}
	if (wnd.kbd.KeyIsPressed('E'))
	{
		input.orbitDelta += orbitSpeed;
	}

	if (wnd.mouse.IsInWindow())
	{
		const auto mousePos = wnd.mouse.GetPos();
		input.mousePosition = { static_cast<float>(mousePos.first), static_cast<float>(mousePos.second) };
		input.hasMousePosition = true;
	}

	while (const auto event = wnd.mouse.Read())
	{
		switch (event->GetType())
		{
		case Mouse::Event::Type::LPress:
			input.attackQueued = true;
			break;
		case Mouse::Event::Type::WheelUp:
			input.zoomDelta -= 1.0f;
			break;
		case Mouse::Event::Type::WheelDown:
			input.zoomDelta += 1.0f;
			break;
		default:
			break;
		}
	}

	return input;
}

void App::ComposeFrame()
{
	wnd.SetTitle(GameplaySimulation::BuildWindowTitle(hudState));

	wnd.Gfx().BeginFrame(0.07f, 0.09f, 0.14f);
	renderer.Render(scene, camera);
	uiRenderer.Render(hudState);
	wnd.Gfx().BeginTextDraw();
	textRenderer.Render(hudState, wnd.GetWidth(), wnd.GetHeight());
	(void)wnd.Gfx().EndTextDraw();
	wnd.Gfx().EndFrame();
}
