#include "KnockbackSystem.h"

#include "../../Engine/Physics/CollisionSystem.h"
#include "../../Engine/Scene/CombatComponent.h"

#include <algorithm>

void KnockbackSystem::Tick(Scene& scene, float dt) noexcept
{
	for (SceneObject& obj : scene.GetObjects())
	{
		if (!obj.combatComponent.has_value())
		{
			continue;
		}

		auto& cc = obj.combatComponent.value();
		if (cc.knockbackTimeRemaining <= 0.0f)
		{
			// Nothing to do. Velocity may have stale non-zero values from
			// a previous burst that was naturally consumed last tick; we
			// don't re-zero here every frame because that would be a
			// no-op write on the common case (no knockback active).
			continue;
		}

		// Slide the actor along the pending velocity. CollisionSystem
		// preserves the existing axis-aligned wall logic, so a knockback
		// into a wall just stops instead of pushing the actor through.
		// y is left untouched — knockback is XZ-only and Y is owned by
		// the controller for movers, fixed at spawn for static actors.
		DirectX::XMFLOAT3 desired = obj.transform.position;
		desired.x += cc.knockbackVelocityXZ.x * dt;
		desired.z += cc.knockbackVelocityXZ.y * dt;
		CollisionSystem::MoveAndSlide(obj, scene, desired);

		cc.knockbackTimeRemaining = std::max(0.0f, cc.knockbackTimeRemaining - dt);
		if (cc.knockbackTimeRemaining <= 0.0f)
		{
			// Stun ended this frame — clear velocity so a future hit
			// that only sets time but not velocity (defensive callers)
			// doesn't replay the previous direction.
			cc.knockbackVelocityXZ = { 0.0f, 0.0f };
		}
	}
}
