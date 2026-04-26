#include "GameplaySimulation.h"
#include "SystemContext.h"
#include <sstream>
#include <string>
#include <utility>

namespace
{
	// Delay between the player crossing into HP <= 0 and the auto-reload of
	// the current scene. Long enough for the "Defeated" text to register, short
	// enough not to feel like punishment.
	constexpr float kDeathReloadDelay = 1.5f;
}

void GameplaySimulation::BuildArena(Scene& scene, TopDownCamera& camera, const std::string& scenePath) noexcept
{
	// Scene JSON's "objective" block (if any) is parsed into `state`; absent
	// block yields a default-constructed state so the system resets cleanly
	// across mid-play reloads.
	ObjectiveState state;
	GameplayArenaBuilder::Build(scene, camera, scenePath, state);
	objectiveSystem.Reset(std::move(state));
}

void GameplaySimulation::ReloadScene(const std::string& scenePath, Scene& scene, TopDownCamera& camera) noexcept
{
	// A reload is semantically a full reset: Scene::Clear (inside Load) wipes
	// all objects including trigger fired-flags, ObjectiveSystem::Reset
	// discards objective progress, and pendingSceneLoad is cleared so we
	// don't trigger a second reload next tick from a stale request. The
	// death-sequence state belongs to the *previous* run, so it resets here
	// too — the new player has full HP and is alive.
	BuildArena(scene, camera, scenePath);
	pendingSceneLoad.reset();
	deathTimer          = 0.0f;
	deathSequenceActive = false;
	pendingDeathReload  = false;
}

std::optional<std::string> GameplaySimulation::ConsumePendingSceneLoad() noexcept
{
	std::optional<std::string> out;
	out.swap(pendingSceneLoad);
	return out;
}

bool GameplaySimulation::ConsumePendingDeathReload() noexcept
{
	const bool consumed = pendingDeathReload;
	pendingDeathReload = false;
	return consumed;
}

const ObjectiveState& GameplaySimulation::GetObjectiveState() const noexcept
{
	return objectiveSystem.GetState();
}

HudState GameplaySimulation::Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept
{
	// System ordering contract (see SystemContext.h for why this is
	// intentionally not data-driven):
	//   1. camera.ApplyInput  — fold orbit/zoom input before anyone reads
	//      the camera.
	//   2. player.Update      — player movement + attack + aim. Must run
	//      before enemy AI which may read player position, and before
	//      trigger checks which key off the player's post-move position.
	//   3. enemy.Update       — enemy AI reacts to the just-moved player.
	//   4. trigger.Update     — checks overlaps using the post-move player
	//      position; may publish LoadSceneRequested.
	//   5. camera.FollowPlayer — snap camera target after the player has
	//      moved so the view stays locked.
	const SystemContext ctx{ input, scene, camera, eventBus, dt };
	cameraSystem.ApplyInput(ctx);
	playerSystem.Update(ctx);
	enemySystem.Update(ctx);
	triggerSystem.Update(ctx);
	cameraSystem.FollowPlayer(ctx);

	// Drain exactly once per tick after all systems have run. Scene-load
	// requests are captured first so they latch even if the drain occurs in
	// the same tick as an EnemyKilled event; ObjectiveSystem still sees the
	// full event batch so a scripted "kill and transition" pair won't drop
	// kill credit on the outgoing scene.
	auto events = eventBus.Drain();
	for (const auto& e : events)
	{
		if (e.type == GameEventType::LoadSceneRequested && !e.a.empty())
		{
			// Last-write-wins if multiple requests queue in a single tick.
			// In practice trigger fireOnce prevents this, but explicit
			// behaviour beats "first request by accident".
			pendingSceneLoad = e.a;
		}
	}
	objectiveSystem.Consume(events);

	HudState hud = BuildHudState(scene);
	hud.objectiveText = objectiveSystem.CurrentText();

	// Death-sequence bookkeeping. HudPresenter sets playerDefeated when the
	// player's combat component crosses to dead; we latch that into a
	// time-counted "show Defeated text, then reload" sequence. Once the
	// pendingDeathReload flag is set, the App's UpdateModel post-pass
	// performs the actual ReloadScene against its tracked currentScenePath.
	if (hud.playerDefeated)
	{
		if (!deathSequenceActive)
		{
			deathSequenceActive = true;
			deathTimer = 0.0f;
		}
		deathTimer += dt;
		if (deathTimer >= kDeathReloadDelay)
		{
			pendingDeathReload = true;
			// Don't clear the sequence-active flag here — ReloadScene resets
			// it. Until then we keep advancing the timer so a missed
			// consume (shouldn't happen) doesn't latch forever.
		}
	}

	return hud;
}

HudState GameplaySimulation::BuildHudState(const Scene& scene) const noexcept
{
	HudState hud = playerSystem.BuildHudState(scene);
	hud.objectiveText = objectiveSystem.CurrentText();
	return hud;
}

std::wstring GameplaySimulation::BuildWindowTitle(const HudState& hudState) noexcept
{
	std::wostringstream oss;
	oss << L"Blue Frog | HP " << static_cast<int>(hudState.playerHealth.current) << L"/" << static_cast<int>(hudState.playerHealth.max);
	if (hudState.hasTarget)
	{
		oss << L" | Enemy " << static_cast<int>(hudState.targetHealth.current) << L"/" << static_cast<int>(hudState.targetHealth.max);
	}
	oss << L" | " << hudState.objectiveText << L" | Q/E orbit | Wheel zoom | F1 gizmos | F2 inspector | F5 reload | F12 save";
	return oss.str();
}
