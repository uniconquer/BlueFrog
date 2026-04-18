#pragma once

#include <DirectXMath.h>
#include <optional>
#include <string>

// Optional effect run when a trigger fires. Recognized `type` values (v1):
//   "log"        — print the standard trigger log line and nothing else.
//                  Implicit when `action` is absent, so scenes remain
//                  backwards-compatible with Phase 5 triggers.
//   "publish"    — emit TriggerFired on the EventBus with `param` as payload
//                  `b` (the tag already carries the scene-defined intent).
//   "load_scene" — emit LoadSceneRequested with `param` as the scene path.
//                  GameplaySimulation reloads the arena after the current
//                  tick so no live iterators get invalidated.
// Validator rejects any other type with a path-prefixed error.
struct TriggerAction
{
	std::string type;
	std::string param;
};

// Axis-aligned XZ trigger volume. The trigger fires when the player's XZ
// position overlaps the half-extents box centred on the owning object's
// transform.position. No render or collision component is required on the
// same object — triggers are "invisible" zones by design.
//
// fireOnce=true (default): fires at most once per scene load and ignores
// subsequent re-entries. Exit detection and persistent state are Phase 6.
struct TriggerComponent
{
	DirectX::XMFLOAT2          halfExtents{ 1.0f, 1.0f }; // X (right), Z (forward)
	std::string                tag;                        // human-readable label for log output
	bool                       fireOnce = true;
	bool                       fired    = false;           // runtime state; reset on Scene::Clear
	std::optional<TriggerAction> action;                   // empty => implicit "log"
};
