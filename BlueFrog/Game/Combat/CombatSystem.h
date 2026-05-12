#pragma once
#include "../../Engine/Scene/SceneObject.h"
#include "../../Engine/UI/DamagePopup.h"
#include <vector>

class EventBus;
class AudioEngine;

class CombatSystem
{
public:
	// `bus` is optional — when non-null, a kill (target crossed from alive to
	// dead in this call) publishes an EnemyKilled event carrying the target's
	// scene name. Callers that don't participate in the event layer may pass
	// nullptr to preserve Phase 5 semantics.
	// `audio` is optional — when non-null, plays an "enemy_hit" SFX when
	// damage is applied, and "enemy_kill" SFX on the alive->dead transition.
	// `popups` is optional — when non-null, every successful damage
	// application appends one DamagePopup anchored at the target's transform
	// position. nullptr disables popup spawning.
	static bool TryMeleeAttack(SceneObject& attacker, SceneObject& target, int damage, float range, EventBus* bus = nullptr, AudioEngine* audio = nullptr, std::vector<DamagePopup>* popups = nullptr) noexcept;
private:
	static void ApplyDamage(SceneObject& target, int damage, EventBus* bus, AudioEngine* audio, std::vector<DamagePopup>* popups) noexcept;
	static float DistanceXZ(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) noexcept;
};
