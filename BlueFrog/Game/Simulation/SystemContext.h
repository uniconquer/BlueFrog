#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Scene/Scene.h"
#include "GameplayInput.h"

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
};
