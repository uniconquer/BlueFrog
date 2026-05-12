#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/DamagePopup.h"
#include "GameplayInput.h"

#include <vector>

class AudioEngine;

// Per-tick parameter bundle passed to every gameplay system's Update call.
//
// Why a struct instead of per-system parameter lists:
// - Adding a new system no longer requires editing its Update signature; it
//   just pulls whatever it needs out of the context.
// - All five systems' Update calls in GameplaySimulation look identical
//   (`system.Update(ctx)`), which makes the tick's ordering contract the
//   sole thing to read when tracing through a frame.
//
// Why NOT a full ISystem registry with data-driven ordering (Phase C
// closure decision): the systems' ordering carries real semantic
// constraints — camera input must run before player movement, player
// before enemy AI that reads player position, trigger after player has
// moved, camera-follow after player has moved. These constraints do not
// map cleanly to a priority number, there is no use case for per-scene
// system toggling, and there are no tests that would benefit. The
// hardcoded call sequence inside GameplaySimulation::Update is the
// intentional answer.
struct SystemContext
{
    const GameplayInput& input;
    Scene&               scene;
    TopDownCamera&       camera;
    EventBus&            eventBus;
    float                dt;
    // Optional audio sink. nullptr when audio init failed or hasn't been
    // wired through to this tick yet — gameplay code calls
    // `if (ctx.audio) ctx.audio->Play(...)` so missing audio degrades
    // silently rather than crashing.
    AudioEngine*         audio = nullptr;
    // Optional sink for transient floating "damage number" popups, owned by
    // App across scene reloads. Combat code appends one DamagePopup per
    // successful damage application; nullptr means the feature is disabled
    // (no popups rendered, no crash).
    std::vector<DamagePopup>* damagePopups = nullptr;
};
