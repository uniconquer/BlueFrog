#include "App.h"

#include "../Engine/Animation/AnimationSystem.h"
#include "../Engine/Scene/SceneSerializer.h"
#include "../Engine/UI/InspectorFields.h"

#include <cstdio>
#include <filesystem>
#include <string>

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
	// Animation tick runs after gameplay (so scene reloads don't leave a
	// stale clipTime on the new instance) and before render so the
	// frame's pose computation reads fresh values.
	AnimationSystem::Tick(scene, dt);
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
		case VK_F2:
			inspectorEnabled = !inspectorEnabled;
			break;
		case VK_F5:
			// Hot-reload: latch the request and let UpdateModel apply it
			// after Update returns, matching the trigger-driven scene-load
			// flow so no system holds live scene-graph references when the
			// reload runs. Multiple presses in one tick coalesce — same
			// last-write-wins semantics as the LoadSceneRequested path.
			reloadRequested = true;
			break;
		case VK_F12:
		{
			// Save: serialize the live scene + camera + objective spec back
			// to currentScenePath. Pairs with F5 (reload) as the editor
			// round-trip — edit via inspector → F12 → F5 → verify on disk.
			std::string err;
			const bool ok = SceneSerializer::Save(
				std::filesystem::path(currentScenePath),
				scene, camera,
				gameplaySimulation.GetObjectiveState(),
				&err);
			if (ok)
			{
				const std::string msg = "[Save] wrote " + currentScenePath + "\n";
				std::fputs(msg.c_str(), stdout);
				::OutputDebugStringA(msg.c_str());
			}
			else
			{
				const std::string msg = "[Save] FAILED: " + err + "\n";
				std::fputs(msg.c_str(), stdout);
				::OutputDebugStringA(msg.c_str());
			}
			break;
		}
		case VK_TAB:
			if (inspectorEnabled)
			{
				const auto& objs = scene.GetObjects();
				const int count = static_cast<int>(objs.size());
				if (count > 0)
				{
					// Shift held = previous, otherwise next. Wrap on both
					// ends so the user never gets stuck at an edge.
					const bool reverse = wnd.kbd.KeyIsPressed(VK_SHIFT);
					inspectorSelected = (inspectorSelected + (reverse ? -1 : 1) + count) % count;
					// New object may not carry the previously-selected field
					// (e.g. picking a wall after editing the Player's HP).
					// Snap to the first available field on the new object.
					inspectorFieldIndex = InspectorFields::FirstAvailable(objs[inspectorSelected]);
				}
			}
			break;
		case VK_PRIOR: // PageUp
		case VK_NEXT:  // PageDown
			if (inspectorEnabled)
			{
				const auto& objs = scene.GetObjects();
				if (!objs.empty())
				{
					const int sel = std::clamp(inspectorSelected, 0, static_cast<int>(objs.size()) - 1);
					const int dir = (e->GetCode() == VK_NEXT) ? 1 : -1;
					inspectorFieldIndex = InspectorFields::CycleAvailable(inspectorFieldIndex, dir, objs[sel]);
				}
			}
			break;
		case VK_LEFT:
		case VK_RIGHT:
			if (inspectorEnabled)
			{
				auto& objs = scene.GetObjects();
				if (!objs.empty())
				{
					const int sel = std::clamp(inspectorSelected, 0, static_cast<int>(objs.size()) - 1);
					const auto kind = static_cast<InspectorFields::Kind>(inspectorFieldIndex);
					if (InspectorFields::IsAvailable(kind, objs[sel]))
					{
						const float sign = (e->GetCode() == VK_RIGHT) ? 1.0f : -1.0f;
						const float scale = wnd.kbd.KeyIsPressed(VK_SHIFT) ? 10.0f : 1.0f;
						const float delta = InspectorFields::DefaultStep(kind) * sign * scale;
						const float current = InspectorFields::GetValue(kind, objs[sel]);
						InspectorFields::SetValue(kind, objs[sel], current + delta);
					}
				}
			}
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

	// Player-death auto-reload. Same path as F5: full ReloadScene against
	// currentScenePath, which clears every gameplay flag including the
	// death-sequence state itself.
	if (gameplaySimulation.ConsumePendingDeathReload())
	{
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
	// Catch any exception escaping the renderer (mesh import failures, etc.)
	// so we can show a diagnostic dialog instead of aborting via
	// std::terminate. The user gets actionable info; we keep the option to
	// keep playing if the per-frame failure is transient.
	try
	{
		renderer.Render(scene, camera);
	}
	catch (const std::exception& e)
	{
		const std::string msg = std::string("Renderer threw: ") + e.what();
		std::fprintf(stderr, "%s\n", msg.c_str());
		::OutputDebugStringA((msg + "\n").c_str());
		::MessageBoxA(nullptr, msg.c_str(), "Renderer error", MB_OK | MB_ICONEXCLAMATION);
		std::exit(1);
	}
	catch (...)
	{
		const char* msg = "Renderer threw an unknown exception";
		::OutputDebugStringA(msg);
		::MessageBoxA(nullptr, msg, "Renderer error", MB_OK | MB_ICONEXCLAMATION);
		std::exit(1);
	}
	if (debugGizmosEnabled)
	{
		// Draw between 3D and 2D so collision/trigger boxes sit in world
		// space but the HUD/text overlays still come on top.
		debugRenderer.Render(scene, camera);
	}
	uiRenderer.Render(hudState);
	wnd.Gfx().BeginTextDraw();
	textRenderer.Render(hudState, wnd.GetWidth(), wnd.GetHeight());
	if (inspectorEnabled)
	{
		textRenderer.RenderInspector(scene, inspectorSelected, inspectorFieldIndex, wnd.GetWidth(), wnd.GetHeight());
	}
	(void)wnd.Gfx().EndTextDraw();
	wnd.Gfx().EndFrame();
}
