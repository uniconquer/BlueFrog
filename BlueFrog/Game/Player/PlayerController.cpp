#include "PlayerController.h"
#include "../../Engine/Physics/CollisionSystem.h"
#include "PlayerAimSystem.h"
#include "../Simulation/GameplaySceneIds.h"
#include "../Combat/CombatSystem.h"
#include <algorithm>

bool PlayerController::Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt, EventBus& bus) noexcept
{
	attackCooldownRemaining = std::max(0.0f, attackCooldownRemaining - dt);

	SceneObject* player = FindPlayer(scene);
	if (player == nullptr || !player->combatComponent.has_value())
	{
		return false;
	}

	if (!player->combatComponent->IsAlive())
	{
		UpdateTint(*player);
		return true;
	}

	const DirectX::XMFLOAT3 move = PlayerMovementSystem::ComputeMoveVector(input, camera);
	DirectX::XMFLOAT3 desiredPosition = player->transform.position;
	desiredPosition.x += move.x * moveSpeed * dt;
	desiredPosition.z += move.z * moveSpeed * dt;
	desiredPosition.y = playerHeight;
	CollisionSystem::MoveAndSlide(*player, scene, desiredPosition);

	DirectX::XMFLOAT3 mouseGroundPoint = player->transform.position;
	if (PlayerAimSystem::ComputeMouseGroundPoint(input, camera, playerHeight, mouseGroundPoint))
	{
		player->transform.rotation.y = PlayerAimSystem::ComputeYawRadians(player->transform.position, mouseGroundPoint);
	}

	if (input.attackQueued && attackCooldownRemaining <= 0.0f)
	{
		TryAttack(scene, *player, bus);
		attackCooldownRemaining = attackCooldown;
	}

	UpdateTint(*player);
	return true;
}

float PlayerController::GetAttackCooldownProgress01() const noexcept
{
	if (attackCooldown <= 0.0f)
	{
		return 1.0f;
	}

	return std::clamp(1.0f - (attackCooldownRemaining / attackCooldown), 0.0f, 1.0f);
}

SceneObject* PlayerController::FindPlayer(Scene& scene) noexcept
{
	return scene.FindObject(GameplaySceneIds::Player);
}

bool PlayerController::TryAttack(Scene& scene, SceneObject& player, EventBus& bus) noexcept
{
	if (SceneObject* enemy = scene.FindObject(GameplaySceneIds::EnemyScout))
	{
		return CombatSystem::TryMeleeAttack(player, *enemy, attackDamage, attackRange, &bus);
	}

	return false;
}

void PlayerController::UpdateTint(SceneObject& player) const noexcept
{
	if (!player.renderComponent.has_value() || !player.renderComponent->material.has_value() || !player.combatComponent.has_value())
	{
		return;
	}

	if (!player.combatComponent->IsAlive())
	{
		player.renderComponent->material->tint = { 0.28f, 0.30f, 0.34f };
		return;
	}

	const float healthRatio = static_cast<float>(player.combatComponent->health) / static_cast<float>(std::max(1, player.combatComponent->maxHealth));
	const float cooldownRatio = attackCooldownRemaining > 0.0f ? attackCooldownRemaining / attackCooldown : 0.0f;
	player.renderComponent->material->tint =
	{
		0.55f + (1.0f - cooldownRatio) * 0.35f,
		0.72f + healthRatio * 0.20f,
		0.30f + healthRatio * 0.25f
	};
}
