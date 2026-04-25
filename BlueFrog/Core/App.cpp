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
	debugRenderer(wnd.Gfx()),
	camera(static_cast<float>(wnd.GetWidth()) / static_cast<float>(wnd.GetHeight()))
{
	currentScenePath = scenePath.empty() ? std::string(kDefaultScenePath) : std::move(scenePath);
	gameplaySimulation.BuildArena(scene, camera, currentScenePath);
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
	PollDebugToggles();
	UpdateModel(dt);
	ComposeFrame();
}

void App::PollDebugToggles() noexcept
{
	// Drain the keyboard event queue and edge-trigger toggles. KeyIsPressed
	// (held-state bitset) used by gameplay code is independent of this queue,
	// so consuming events here does not break movement input.
	while (const auto e = wnd.kbd.ReadKey())
	{
		if (!e->IsPress())
		{
			continue;
		}
		switch (e->GetCode())
		{
		case VK_F1:
			debugGizmosEnabled = !debugGizmosEnabled;
			break;
		case VK_F5:
			// Hot-reload: latch the request and let UpdateModel apply it
			// after Update returns, matching the trigger-driven scene-load
			// flow so no system holds live scene-graph references when the
			// reload runs. Multiple presses in one tick coalesce — same
			// last-write-wins semantics as the LoadSceneRequested path.
			reloadRequested = true;
			break;
		default:
			break;
		}
	}
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
		// Trigger-driven scene transition. Capture the new path so a
		// subsequent F5 reloads the *new* scene, not the previous one.
		currentScenePath = *path;
		gameplaySimulation.ReloadScene(currentScenePath, scene, camera);
		hudState = gameplaySimulation.BuildHudState(scene);
	}

	// F5 hot-reload runs after the trigger-driven path so a same-tick combo
	// (rare: triggered transition AND F5 in one frame) ends up reloading the
	// scene we just transitioned into, which is what the developer pressing
	// F5 in that frame would visually expect.
	if (reloadRequested)
	{
		reloadRequested = false;
		gameplaySimulation.ReloadScene(currentScenePath, scene, camera);
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
	if (debugGizmosEnabled)
	{
		// Draw between 3D and 2D so collision/trigger boxes sit in world
		// space but the HUD/text overlays still come on top.
		debugRenderer.Render(scene, camera);
	}
	uiRenderer.Render(hudState);
	wnd.Gfx().BeginTextDraw();
	textRenderer.Render(hudState, wnd.GetWidth(), wnd.GetHeight());
	(void)wnd.Gfx().EndTextDraw();
	wnd.Gfx().EndFrame();
}
