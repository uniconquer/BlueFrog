#include "SimpleEnemyController.h"
#include "../../Engine/Physics/CollisionSystem.h"
#include "../Combat/CombatSystem.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

void SimpleEnemyController::Update(Scene& scene, float dt) noexcept
{
	attackCooldownRemaining = std::max(0.0f, attackCooldownRemaining - dt);

	SceneObject* player = scene.FindObject("Player");
	SceneObject* enemy = FindEnemy(scene);
	if (player == nullptr || enemy == nullptr || !player->combatComponent.has_value() || !enemy->combatComponent.has_value())
	{
		return;
	}

	if (!enemy->combatComponent->IsAlive())
	{
		UpdateTint(*enemy, false);
		return;
	}

	if (!player->combatComponent->IsAlive())
	{
		UpdateTint(*enemy, false);
		return;
	}

	const float dx = player->transform.position.x - enemy->transform.position.x;
	const float dz = player->transform.position.z - enemy->transform.position.z;
	const float distance = std::sqrt(dx * dx + dz * dz);
	const bool chasing = distance <= chaseRange;

	UpdateTint(*enemy, chasing);

	if (!chasing || distance < 0.001f)
	{
		return;
	}

	enemy->transform.rotation.y = ComputeYawRadians(enemy->transform.position, player->transform.position);

	if (distance > attackRange)
	{
		const float invDistance = 1.0f / distance;
		XMFLOAT3 desiredPosition = enemy->transform.position;
		desiredPosition.x += dx * invDistance * moveSpeed * dt;
		desiredPosition.z += dz * invDistance * moveSpeed * dt;
		CollisionSystem::MoveAndSlide(*enemy, scene, desiredPosition);
		return;
	}

	if (attackCooldownRemaining <= 0.0f && CombatSystem::TryMeleeAttack(*enemy, *player, attackDamage, attackRange + 0.2f))
	{
		attackCooldownRemaining = attackCooldown;
	}
}

SceneObject* SimpleEnemyController::FindEnemy(Scene& scene) noexcept
{
	return scene.FindObject("EnemyScout");
}

float SimpleEnemyController::ComputeYawRadians(const XMFLOAT3& from, const XMFLOAT3& to) noexcept
{
	return std::atan2(to.x - from.x, to.z - from.z);
}

void SimpleEnemyController::UpdateTint(SceneObject& enemy, bool chasing) const noexcept
{
	if (!enemy.renderComponent.has_value())
	{
		return;
	}

	if (!enemy.combatComponent.has_value() || !enemy.combatComponent->IsAlive())
	{
		enemy.renderComponent->tint = { 0.30f, 0.32f, 0.36f };
		return;
	}

	enemy.renderComponent->tint = chasing ? DirectX::XMFLOAT3{ 1.0f, 0.50f, 0.42f } : DirectX::XMFLOAT3{ 0.92f, 0.36f, 0.36f };
}
