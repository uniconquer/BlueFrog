#pragma once
#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/DamagePopup.h"
#include "PlayerMovementSystem.h"
#include "../Simulation/GameplayInput.h"
#include <vector>

class EventBus;
class AudioEngine;

class PlayerController
{
public:
	bool Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt, EventBus& bus, AudioEngine* audio, std::vector<DamagePopup>* popups) noexcept;
	float GetAttackCooldownProgress01() const noexcept;
private:
	SceneObject* FindPlayer(Scene& scene) noexcept;
	bool TryAttack(Scene& scene, SceneObject& player, EventBus& bus, AudioEngine* audio, std::vector<DamagePopup>* popups) noexcept;
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

	// Dash: short burst at higher speed in the current movement direction
	// (or facing if no movement input). Fixed window of dashDuration with
	// a recovery window of dashCooldown after — held-key sampling means
	// the player can chain dashes back-to-back at the cooldown rate.
	static constexpr float dashSpeedMul   = 3.0f; // multiplied by moveSpeed
	static constexpr float dashDuration   = 0.18f;
	static constexpr float dashCooldown   = 0.55f;
	float dashTimeRemaining     = 0.0f;
	float dashCooldownRemaining = 0.0f;
	float dashDirX              = 0.0f;
	float dashDirZ              = 0.0f;
};
