#pragma once
#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "PlayerMovementSystem.h"
#include "../Simulation/GameplayInput.h"

class EventBus;

class PlayerController
{
public:
	bool Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt, EventBus& bus) noexcept;
	float GetAttackCooldownProgress01() const noexcept;
private:
	SceneObject* FindPlayer(Scene& scene) noexcept;
	bool TryAttack(Scene& scene, SceneObject& player, EventBus& bus) noexcept;
	void UpdateTint(SceneObject& player) const noexcept;
private:
	static constexpr float moveSpeed = 6.5f;
	// Y position the player snaps to each tick. 0 = feet on the ground
	// plane (Phase F Stage 4c switched the player from a center-pivoted
	// cube to a feet-pivoted skinned character mesh).
	static constexpr float playerHeight = 0.0f;
	static constexpr float attackRange = 2.4f;
	static constexpr float attackCooldown = 0.45f;
	static constexpr int attackDamage = 1;
	float attackCooldownRemaining = 0.0f;
};
