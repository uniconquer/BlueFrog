#include "FLApp.h"

#include "../Game/Simulation/AnimationControllerSystem.h"
#include "../Engine/Animation/AnimationSystem.h"
#include "../Engine/Scene/SceneSerializer.h"
#include "../Engine/UI/InspectorFields.h"
#include "../Game/Objectives/ObjectiveStateIO.h"
#include "../Game/Simulation/GameplaySceneIds.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace
{
	constexpr const char* kDefaultScenePath = "Assets/Scenes/arena_trial.json";

	// ASCII narrow→wide for HUD display. NPC text is validator-bound to
	// ASCII per the scene schema, so a 1:1 widen is correct.
	std::wstring Widen(const std::string& s)
	{
		std::wstring out;
		out.reserve(s.size());
		for (char c : s)
		{
			out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
		}
		return out;
	}
}

FLApp::FLApp(std::string scenePath)
	:
	AppBase(800, 600, L"FL"),
	renderer(GetGfx()),
	uiRenderer(GetGfx()),
	textRenderer(GetGfx()),
	debugRenderer(GetGfx()),
	worldGridRenderer(GetGfx()),
	camera(static_cast<float>(GetWindow().GetWidth()) / static_cast<float>(GetWindow().GetHeight())),
	currentScenePath(scenePath.empty() ? std::string(kDefaultScenePath) : std::move(scenePath))
{
	// ctor only does member init that needs the base class alive. The
	// rest of the boot sequence (profile load, BuildArena, audio init)
	// lives in OnStartup so it's clear what "game becomes ready" means.
}

void FLApp::OnStartup()
{
	// Auto-load profile if present. Profile-supplied scene path overrides
	// the CLI / default; HP override is applied AFTER BuildArena so the
	// scene-spawned HP is replaced by the saved value.
	PlayerProfile profile;
	const std::filesystem::path profilePath("Save/profile.json");
	const bool profileLoaded = PlayerProfileIO::Load(profilePath, profile);
	if (profileLoaded && !profile.scenePath.empty())
	{
		currentScenePath = profile.scenePath;
	}

	gameplaySimulation.BuildArena(scene, camera, currentScenePath);

	if (profileLoaded && profile.playerHealth > 0)
	{
		if (SceneObject* player = scene.FindObject(GameplaySceneIds::Player))
		{
			if (player->combatComponent.has_value())
			{
				if (profile.playerMaxHealth > 0)
				{
					player->combatComponent->maxHealth = profile.playerMaxHealth;
				}
				player->combatComponent->health = profile.playerHealth;
			}
		}
		currentPlayTimeSec = profile.playTimeSec;
	}

	hudState = gameplaySimulation.BuildHudState(scene);

	// Audio: load placeholder SFX once at boot. Failures are logged via
	// OutputDebugString and silently fall through — Play() becomes a no-op
	// for the missing slot, gameplay continues.
	audio.LoadSound("attack",     std::filesystem::path("Assets/Audio/attack.wav"));
	audio.LoadSound("enemy_hit",  std::filesystem::path("Assets/Audio/enemy_hit.wav"));
	audio.LoadSound("enemy_kill", std::filesystem::path("Assets/Audio/enemy_kill.wav"));
	audio.LoadBgm("arena", std::filesystem::path("Assets/Audio/bgm_arena.wav"));
	audio.PlayBgm("arena");
	gameplaySimulation.SetAudio(&audio);
	gameplaySimulation.SetDamagePopupSink(&activePopups);
}

void FLApp::OnUpdate(float dt)
{
	currentPlayTimeSec += dt;
	PollDebugToggles();
	const GameplayInput input = CollectGameplayInput(dt);

	// Dialog state transitions (Phase I-1C). E toggles: in dialog → exit;
	// out of dialog + prompt visible → enter, capturing the NPC payload
	// from the previous tick's InteractionSystem scan. Must happen
	// BEFORE UpdateModel so the dialogActive flag we push into
	// GameplaySimulation reflects this frame's decision — otherwise the
	// prompt would flicker for one tick after dialog open.
	if (input.interactPressed)
	{
		if (dialogActive)
		{
			dialogActive = false;
			dialogNpcName.clear();
			dialogText.clear();
		}
		else if (const SceneObject* npc = gameplaySimulation.GetInteractTarget())
		{
			if (npc->npcComponent.has_value())
			{
				dialogActive = true;
				const auto& nc = npc->npcComponent.value();
				dialogNpcName = Widen(nc.displayName.empty() ? npc->name : nc.displayName);
				dialogText    = Widen(nc.dialogText);
			}
		}
	}
	gameplaySimulation.SetDialogActive(dialogActive);

	UpdateModel(input, dt);

	// Damage flash bookkeeping. Detect player HP drop between ticks; if
	// so kick the flash to peak. Decay it every tick toward zero. Reset
	// the baseline whenever the player respawns above the previous low
	// (typically after a scene reload).
	const int curHp = static_cast<int>(hudState.playerHealth.current);
	const bool playerTookDamage = (lastPlayerHealth >= 0 && curHp < lastPlayerHealth);
	if (playerTookDamage)
	{
		damageFlashAlpha = 1.0f;
	}
	lastPlayerHealth = curHp;
	constexpr float kDamageFlashDuration = 0.35f;
	damageFlashAlpha = std::max(0.0f, damageFlashAlpha - dt / kDamageFlashDuration);

	// Screen-shake kickers. Heavy kick on player taking damage; lighter
	// kick on any newly-spawned damage popup (i.e. *something* got hit).
	// The popup-delta check is OR'd after the HP check so a hit that
	// damaged the player doesn't double-trigger as both "took damage"
	// AND "landed a hit on player". Direction is a random unit-vector
	// in XZ so consecutive kicks don't reinforce one axis.
	const bool popupCountGrew = (activePopups.size() > lastPopupCount);
	if (playerTookDamage)
	{
		shakeMagnitude = std::max(shakeMagnitude, 0.35f);
		shakeTimer = 0.0f;
		const float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 6.2831853f;
		shakeDirX = std::cos(angle);
		shakeDirZ = std::sin(angle);
	}
	else if (popupCountGrew)
	{
		shakeMagnitude = std::max(shakeMagnitude, 0.14f);
		shakeTimer = 0.0f;
		const float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 6.2831853f;
		shakeDirX = std::cos(angle);
		shakeDirZ = std::sin(angle);
	}
	lastPopupCount = activePopups.size();
	// Magnitude decays linearly over ~0.22s; timer always advances so the
	// sin oscillation runs at fixed temporal frequency rather than warping
	// with magnitude. Both reset cleanly when the next kick lands.
	constexpr float kShakeDecayPerSec = 1.0f / 0.22f;
	shakeMagnitude = std::max(0.0f, shakeMagnitude - dt * 0.35f * kShakeDecayPerSec);
	shakeTimer += dt;

	// Tick damage-popup ages and erase the ones past their lifetime. Done
	// AFTER UpdateModel (which is where combat code appends new popups via
	// the SystemContext sink) so a popup spawned this tick gets a full
	// frame of render time before its age is advanced.
	for (DamagePopup& p : activePopups)
	{
		p.age += dt;
	}
	activePopups.erase(
		std::remove_if(activePopups.begin(), activePopups.end(),
			[](const DamagePopup& p) noexcept { return p.age >= DamagePopupConstants::kMaxAge; }),
		activePopups.end());

	// Animation controller picks clipName based on the gameplay state we
	// just settled (player movement intent, enemy distance, alive flag).
	// Runs BEFORE AnimationSystem::Tick so the time-advance step uses the
	// freshly-selected clip's duration.
	AnimationControllerSystem::Tick(scene, input, dt);
	AnimationSystem::Tick(scene, dt);
}

void FLApp::PollDebugToggles() noexcept
{
	// Drain the keyboard event queue and edge-trigger toggles. KeyIsPressed
	// (held-state bitset) used by gameplay code is independent of this queue,
	// so consuming events here does not break movement input.
	while (const auto e = GetKeyboard().ReadKey())
	{
		if (!e->IsPress())
		{
			continue;
		}
		switch (e->GetCode())
		{
		case 'E':
			// Dialog interact key. Latched here as an edge (press-only,
			// since we already filtered to IsPress() above) so dialog
			// open/close is one transition per physical keystroke. The
			// flag is consumed and cleared in CollectGameplayInput.
			interactPressedThisFrame = true;
			break;
		case VK_F1:
			debugGizmosEnabled = !debugGizmosEnabled;
			break;
		case VK_F2:
			inspectorEnabled = !inspectorEnabled;
			break;
		case VK_F3:
			worldGridEnabled = !worldGridEnabled;
			break;
		case VK_F5:
			// Hot-reload: latch the request and let UpdateModel apply it
			// after Update returns, matching the trigger-driven scene-load
			// flow so no system holds live scene-graph references when the
			// reload runs. Multiple presses in one tick coalesce — same
			// last-write-wins semantics as the LoadSceneRequested path.
			reloadRequested = true;
			break;
		case VK_F8:
		{
			// Profile save: snapshot current scene + player HP + accumulated
			// play time into Save/profile.json. Subsequent app launches
			// auto-load this and start from the saved scene with the saved
			// HP applied on top of the spawn defaults.
			PlayerProfile profile;
			profile.scenePath = currentScenePath;
			profile.playTimeSec = currentPlayTimeSec;
			if (const SceneObject* player = scene.FindObject(GameplaySceneIds::Player))
			{
				if (player->combatComponent.has_value())
				{
					profile.playerHealth    = player->combatComponent->health;
					profile.playerMaxHealth = player->combatComponent->maxHealth;
				}
			}
			const std::filesystem::path profilePath("Save/profile.json");
			const bool ok = PlayerProfileIO::Save(profilePath, profile);
			const std::string msg = ok
				? std::string("[Profile] saved to ") + profilePath.string() + "\n"
				: std::string("[Profile] save FAILED\n");
			std::fputs(msg.c_str(), stdout);
			::OutputDebugStringA(msg.c_str());
			break;
		}
		case VK_F12:
		{
			// Save: serialize the live scene + camera + objective spec back
			// to currentScenePath. Pairs with F5 (reload) as the editor
			// round-trip — edit via inspector → F12 → F5 → verify on disk.
			std::string err;
			// SceneSerializer is engine-agnostic about objectives — encode
			// to raw JSON text here on the game side, then hand it over.
			const std::string objectiveBlockJson = ObjectiveStateIO::EncodeJson(gameplaySimulation.GetObjectiveState());
			const bool ok = SceneSerializer::Save(
				std::filesystem::path(currentScenePath),
				scene, camera,
				objectiveBlockJson,
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
					const bool reverse = GetKeyboard().KeyIsPressed(VK_SHIFT);
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
						const float scale = GetKeyboard().KeyIsPressed(VK_SHIFT) ? 10.0f : 1.0f;
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

void FLApp::UpdateModel(const GameplayInput& input, float dt) noexcept
{
	hudState = gameplaySimulation.Update(input, scene, camera, dt);

	// Honor LoadSceneRequested events drained during Update. Reload happens
	// here — after every system has finished iterating the scene — so no
	// live references into scene objects are invalidated mid-tick. We
	// immediately re-BuildHudState against the new scene to avoid a
	// 1-frame-stale title bar (same pattern as FLApp's OnStartup).
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

GameplayInput FLApp::CollectGameplayInput(float dt) noexcept
{
	const float orbitSpeed = 1.2f * dt;
	GameplayInput input = {};
	input.viewportSize = { static_cast<float>(GetWindow().GetWidth()), static_cast<float>(GetWindow().GetHeight()) };
	input.movementIntent =
	{
		(GetKeyboard().KeyIsPressed('D') ? 1.0f : 0.0f) - (GetKeyboard().KeyIsPressed('A') ? 1.0f : 0.0f),
		(GetKeyboard().KeyIsPressed('W') ? 1.0f : 0.0f) - (GetKeyboard().KeyIsPressed('S') ? 1.0f : 0.0f)
	};

	// Camera orbit: Q rotates left, R rotates right. R replaced the
	// original E binding so E is interact-only — pressing E to talk no
	// longer also nudges the camera.
	if (GetKeyboard().KeyIsPressed('Q'))
	{
		input.orbitDelta -= orbitSpeed;
	}
	if (GetKeyboard().KeyIsPressed('R'))
	{
		input.orbitDelta += orbitSpeed;
	}
	// Dash gate. PlayerController internal cooldown handles the "no spam"
	// requirement, so plain held-key sampling is enough here.
	input.dashHeld = GetKeyboard().KeyIsPressed(VK_SPACE);

	// E-key edge (set during PollDebugToggles). Consume + clear so the
	// next tick starts with a fresh flag.
	input.interactPressed = interactPressedThisFrame;
	interactPressedThisFrame = false;

	if (GetMouse().IsInWindow())
	{
		const auto mousePos = GetMouse().GetPos();
		input.mousePosition = { static_cast<float>(mousePos.first), static_cast<float>(mousePos.second) };
		input.hasMousePosition = true;
	}

	while (const auto event = GetMouse().Read())
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

void FLApp::OnRender()
{
	GetWindow().SetTitle(GameplaySimulation::BuildWindowTitle(hudState));

	// Push the live shake offset into the camera RIGHT before any view
	// matrix is read this frame. ~32Hz oscillation (timer*200) feels
	// "crunchy" — much lower reads as a slow swing, much higher as
	// noise. Cleared at the end of OnRender so any code path that
	// peeks the view matrix outside the render block sees a stable
	// camera.
	const float shakeOsc = std::sin(shakeTimer * 200.0f) * shakeMagnitude;
	camera.SetShakeOffsetXZ(shakeDirX * shakeOsc, shakeDirZ * shakeOsc);

	GetGfx().BeginFrame(0.07f, 0.09f, 0.14f);
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
	if (worldGridEnabled)
	{
		// Ground reference grid (Unity-style). Drawn after the 3D pass and
		// before debug gizmos so trigger/collision lines sit visually on
		// top of the grid lines they share screen space with.
		worldGridRenderer.Render(camera);
	}
	if (debugGizmosEnabled)
	{
		// Draw between 3D and 2D so collision/trigger boxes sit in world
		// space but the HUD/text overlays still come on top.
		debugRenderer.Render(scene, camera);
	}
	uiRenderer.Render(hudState);
	GetGfx().BeginTextDraw();
	textRenderer.Render(hudState, GetWindow().GetWidth(), GetWindow().GetHeight(), inspectorEnabled, damageFlashAlpha);
	// Interaction prompt: drawn between persistent HUD text and popups
	// so the "[E] Talk to X" bottom hint sits cleanly above other
	// transient overlays without fighting the inspector for column space.
	// Skipped while a dialog is open — the dialog box already covers
	// that screen real estate.
	if (!dialogActive)
	{
		textRenderer.RenderInteractPrompt(hudState, GetWindow().GetWidth(), GetWindow().GetHeight());
	}
	if (dialogActive)
	{
		textRenderer.RenderDialog(dialogNpcName, dialogText, GetWindow().GetWidth(), GetWindow().GetHeight());
	}
	// Damage-number overlay: drawn between the persistent HUD text and the
	// inspector panel so the panel (if open) still covers the right-side
	// popups, keeping the editor view tidy.
	textRenderer.RenderDamagePopups(activePopups, camera, GetWindow().GetWidth(), GetWindow().GetHeight());
	if (inspectorEnabled)
	{
		textRenderer.RenderInspector(scene, inspectorSelected, inspectorFieldIndex, GetWindow().GetWidth(), GetWindow().GetHeight());
	}
	(void)GetGfx().EndTextDraw();
	GetGfx().EndFrame();
	camera.SetShakeOffsetXZ(0.0f, 0.0f);
}
