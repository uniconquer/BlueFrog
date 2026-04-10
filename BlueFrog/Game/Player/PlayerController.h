#pragma once
#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "PlayerMovementSystem.h"
#include "../Simulation/GameplayInput.h"

class PlayerController
{
public:
	bool Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept;
	float GetAttackCooldownProgress01() const noexcept;
private:
	SceneObject* FindPlayer(Scene& scene) noexcept;
	bool TryAttack(Scene& scene, SceneObject& player) noexcept;
	void UpdateTint(SceneObject& player) const noexcept;
private:
	static constexpr float moveSpeed = 6.5f;
	static constexpr float playerHeight = 1.25f;
	static constexpr float attackRange = 2.4f;
	static constexpr float attackCooldown = 0.45f;
	static constexpr int attackDamage = 1;
	float attackCooldownRemaining = 0.0f;
};
