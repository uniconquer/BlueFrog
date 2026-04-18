#pragma once

#include <DirectXMath.h>
#include <string>

// Axis-aligned XZ trigger volume. The trigger fires when the player's XZ
// position overlaps the half-extents box centred on the owning object's
// transform.position. No render or collision component is required on the
// same object — triggers are "invisible" zones by design.
//
// fireOnce=true (default): fires at most once per scene load and ignores
// subsequent re-entries. Exit detection and persistent state are Phase 6.
struct TriggerComponent
{
	DirectX::XMFLOAT2 halfExtents{ 1.0f, 1.0f }; // X (right), Z (forward)
	std::string        tag;                        // human-readable label for log output
	bool               fireOnce = true;
	bool               fired    = false;           // runtime state; reset on Scene::Clear
};
