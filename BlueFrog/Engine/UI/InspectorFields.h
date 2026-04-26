#pragma once

#include "../Scene/SceneObject.h"

#include <algorithm>

// Shared metadata for the inspector's edit mode. App reads/writes values via
// these helpers when the user presses arrow keys; TextRenderer uses the same
// helpers so the highlighted "edit cursor" row in the panel stays in sync
// with what the keyboard actually mutates.
//
// To add a new editable field:
//   1. Add a new enum value before COUNT.
//   2. Add a case in IsAvailable / GetValue / SetValue / Step / DisplayName.
//   3. (Optional) Adjust DefaultStep magnitude.
//
// The current set covers the daily-tweak fields (transform.position, yaw,
// combat HP). Tint, scale, halfExtents, and trigger flags are left as future
// extensions; the point of v1 is to demonstrate the read-write loop, not to
// be exhaustive.
namespace InspectorFields
{
    enum class Kind
    {
        PosX,
        PosY,
        PosZ,
        RotY,
        CombatHP,
        COUNT
    };

    inline bool IsAvailable(Kind k, const SceneObject& obj) noexcept
    {
        switch (k)
        {
        case Kind::PosX:
        case Kind::PosY:
        case Kind::PosZ:
        case Kind::RotY:
            return true;
        case Kind::CombatHP:
            return obj.combatComponent.has_value();
        default:
            return false;
        }
    }

    inline float GetValue(Kind k, const SceneObject& obj) noexcept
    {
        switch (k)
        {
        case Kind::PosX: return obj.transform.position.x;
        case Kind::PosY: return obj.transform.position.y;
        case Kind::PosZ: return obj.transform.position.z;
        case Kind::RotY: return obj.transform.rotation.y;
        case Kind::CombatHP:
            return obj.combatComponent.has_value()
                ? static_cast<float>(obj.combatComponent->health)
                : 0.0f;
        default:
            return 0.0f;
        }
    }

    inline void SetValue(Kind k, SceneObject& obj, float v) noexcept
    {
        switch (k)
        {
        case Kind::PosX: obj.transform.position.x = v; break;
        case Kind::PosY: obj.transform.position.y = v; break;
        case Kind::PosZ: obj.transform.position.z = v; break;
        case Kind::RotY: obj.transform.rotation.y = v; break;
        case Kind::CombatHP:
            if (obj.combatComponent.has_value())
            {
                // Round to int and clamp to [0, maxHealth]. Crossing to
                // 0 here is a legitimate way to test the death sequence
                // from the inspector — drop a Player's HP and watch the
                // auto-reload kick in.
                const int rounded = static_cast<int>(v + (v >= 0.0f ? 0.5f : -0.5f));
                obj.combatComponent->health = std::clamp(rounded, 0, obj.combatComponent->maxHealth);
            }
            break;
        default: break;
        }
    }

    inline float DefaultStep(Kind k) noexcept
    {
        switch (k)
        {
        case Kind::PosX:
        case Kind::PosY:
        case Kind::PosZ:
            return 0.10f;
        case Kind::RotY:
            return 0.10f; // radians, ~5.7°
        case Kind::CombatHP:
            return 1.0f;
        default:
            return 1.0f;
        }
    }

    inline const wchar_t* DisplayName(Kind k) noexcept
    {
        switch (k)
        {
        case Kind::PosX:     return L"pos.x";
        case Kind::PosY:     return L"pos.y";
        case Kind::PosZ:     return L"pos.z";
        case Kind::RotY:     return L"rot.y";
        case Kind::CombatHP: return L"combat.hp";
        default:             return L"?";
        }
    }

    // Advance `current` by `direction` (+1 or -1), skipping fields that are
    // not available on `obj`. Wraps at both ends. If no field is available
    // (all components missing — only the player can hit this in pathological
    // scenes), returns the input unchanged.
    inline int CycleAvailable(int current, int direction, const SceneObject& obj) noexcept
    {
        const int N = static_cast<int>(Kind::COUNT);
        if (N <= 0) return current;
        for (int tries = 0; tries < N; ++tries)
        {
            current = (current + direction + N) % N;
            if (IsAvailable(static_cast<Kind>(current), obj))
            {
                return current;
            }
        }
        return current;
    }

    // Find the first available field index >= 0. Used after Tab cycles to
    // a new object, since the previously-selected field may not exist on
    // the newly-selected object.
    inline int FirstAvailable(const SceneObject& obj) noexcept
    {
        const int N = static_cast<int>(Kind::COUNT);
        for (int i = 0; i < N; ++i)
        {
            if (IsAvailable(static_cast<Kind>(i), obj))
            {
                return i;
            }
        }
        return 0;
    }
}
