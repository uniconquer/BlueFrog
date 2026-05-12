#pragma once

#include <DirectXMath.h>

// Transient floating "damage number" particle, spawned by CombatSystem on
// every successful damage application and drained by TextRenderer once per
// frame. Owned by App (so the vector survives across scene reloads and isn't
// torn down with any one system) and threaded into gameplay through the
// SystemContext::damagePopups pointer.
//
// Kept deliberately POD so it can sit in a std::vector without ctor/move
// considerations. TextRenderer projects worldPos via the live camera each
// frame; nothing here is precomputed because the camera is free to orbit
// mid-popup and the number should track its enemy's screen position
// (well, the position at spawn — the popup does NOT follow a live target
// because the target object may already be dead/freed by then).
struct DamagePopup
{
    DirectX::XMFLOAT3 worldPos = { 0.0f, 0.0f, 0.0f };
    int               amount   = 0;
    float             age      = 0.0f;
};

namespace DamagePopupConstants
{
    // Total lifetime in seconds. Long enough to read at a glance, short
    // enough that several stacked popups don't visually pile up.
    inline constexpr float kMaxAge = 0.9f;

    // Vertical rise (in DIP) over the popup's lifetime. Tuned so the
    // popup ends up roughly one bar-height above its spawn point.
    inline constexpr float kFloatUpDip = 56.0f;

    // Extra world-space vertical offset above worldPos at spawn time so the
    // number appears above the enemy's head instead of its feet.
    inline constexpr float kSpawnYOffset = 1.6f;
}
