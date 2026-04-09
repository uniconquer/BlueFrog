#pragma once
#include "../../Engine/Scene/Scene.h"

class SimpleEnemyController
{
public:
	void Update(Scene& scene, float dt) noexcept;
private:
	static SceneObject* FindEnemy(Scene& scene) noexcept;
	static float ComputeYawRadians(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to) noexcept;
	void UpdateTint(SceneObject& enemy, bool chasing) const noexcept;
private:
	float attackCooldownRemaining = 0.0f;
	static constexpr float moveSpeed = 2.8f;
	static constexpr float chaseRange = 12.0f;
	static constexpr float attackRange = 1.8f;
	static constexpr float attackCooldown = 1.15f;
	static constexpr int attackDamage = 1;
};
