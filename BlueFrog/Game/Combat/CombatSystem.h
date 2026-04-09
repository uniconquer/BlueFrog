#pragma once
#include "../../Engine/Scene/SceneObject.h"

class CombatSystem
{
public:
	static bool TryMeleeAttack(SceneObject& attacker, SceneObject& target, int damage, float range) noexcept;
private:
	static void ApplyDamage(SceneObject& target, int damage) noexcept;
	static float DistanceXZ(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) noexcept;
};
