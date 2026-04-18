#pragma once
#include "../../Engine/Scene/SceneObject.h"

class EventBus;

class CombatSystem
{
public:
	// `bus` is optional — when non-null, a kill (target crossed from alive to
	// dead in this call) publishes an EnemyKilled event carrying the target's
	// scene name. Callers that don't participate in the event layer may pass
	// nullptr to preserve Phase 5 semantics.
	static bool TryMeleeAttack(SceneObject& attacker, SceneObject& target, int damage, float range, EventBus* bus = nullptr) noexcept;
private:
	static void ApplyDamage(SceneObject& target, int damage, EventBus* bus) noexcept;
	static float DistanceXZ(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) noexcept;
};
