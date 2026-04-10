#include "PlayerController.h"
#include "../../Engine/Physics/CollisionSystem.h"
#include "../Combat/CombatSystem.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	XMFLOAT3 MakeMoveVector(float strafe, float forward, const TopDownCamera& camera) noexcept
	{
		float x = std::clamp(strafe, -1.0f, 1.0f);
		float z = std::clamp(forward, -1.0f, 1.0f);

		const XMFLOAT3 cameraTarget = camera.GetTarget();
		const XMFLOAT3 cameraPos = camera.GetPosition();
		const float forwardX = cameraTarget.x - cameraPos.x;
		const float forwardZ = cameraTarget.z - cameraPos.z;
		const float forwardLength = std::sqrt(forwardX * forwardX + forwardZ * forwardZ);

		if (forwardLength > 0.0001f)
		{
			const float normalizedForwardX = forwardX / forwardLength;
			const float normalizedForwardZ = forwardZ / forwardLength;
			const float rightX = normalizedForwardZ;
			const float rightZ = -normalizedForwardX;
			const float moveX = rightX * x + normalizedForwardX * z;
			const float moveZ = rightZ * x + normalizedForwardZ * z;

			const float length = std::sqrt(moveX * moveX + moveZ * moveZ);
			if (length > 0.0f)
			{
				return { moveX / length, 0.0f, moveZ / length };
			}
		}

		const float length = std::sqrt(x * x + z * z);
		if (length > 0.0f)
		{
			x /= length;
			z /= length;
		}

		return { x, 0.0f, z };
	}
}

bool PlayerController::Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept
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

	const XMFLOAT3 move = GetMoveVector(input, camera);
	XMFLOAT3 desiredPosition = player->transform.position;
	desiredPosition.x += move.x * moveSpeed * dt;
	desiredPosition.z += move.z * moveSpeed * dt;
	desiredPosition.y = playerHeight;
	CollisionSystem::MoveAndSlide(*player, scene, desiredPosition);

	XMFLOAT3 mouseGroundPoint = player->transform.position;
	if (ComputeMouseGroundPoint(input, camera, mouseGroundPoint))
	{
		player->transform.rotation.y = ComputePlayerYawRadians(player->transform.position, mouseGroundPoint);
	}

	if (input.attackQueued && attackCooldownRemaining <= 0.0f)
	{
		TryAttack(scene, *player);
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
	return scene.FindObject("Player");
}

XMFLOAT3 PlayerController::GetMoveVector(const GameplayInput& input, const TopDownCamera& camera) noexcept
{
	return MakeMoveVector(input.movementIntent.x, input.movementIntent.y, camera);
}

bool PlayerController::ComputeMouseGroundPoint(const GameplayInput& input, const TopDownCamera& camera, XMFLOAT3& outPoint) noexcept
{
	if (!input.hasMousePosition)
	{
		return false;
	}

	const float width = std::max(1.0f, input.viewportSize.x);
	const float height = std::max(1.0f, input.viewportSize.y);
	const float ndcX = (input.mousePosition.x / width) * 2.0f - 1.0f;
	const float ndcY = 1.0f - (input.mousePosition.y / height) * 2.0f;

	const XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();
	const XMMATRIX inverseViewProjection = XMMatrixInverse(nullptr, viewProjection);

	const XMVECTOR nearPoint = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), inverseViewProjection);
	const XMVECTOR farPoint = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), inverseViewProjection);
	const XMVECTOR ray = XMVectorSubtract(farPoint, nearPoint);
	const float rayY = XMVectorGetY(ray);
	if (std::fabs(rayY) < 0.0001f)
	{
		return false;
	}

	const float t = -XMVectorGetY(nearPoint) / rayY;
	if (t < 0.0f)
	{
		return false;
	}

	const XMVECTOR worldPoint = XMVectorAdd(nearPoint, XMVectorScale(ray, t));
	XMStoreFloat3(&outPoint, worldPoint);
	outPoint.y = playerHeight;
	return true;
}

bool PlayerController::TryAttack(Scene& scene, SceneObject& player) noexcept
{
	if (SceneObject* enemy = scene.FindObject("EnemyScout"))
	{
		return CombatSystem::TryMeleeAttack(player, *enemy, attackDamage, attackRange);
	}

	return false;
}

void PlayerController::UpdateTint(SceneObject& player) const noexcept
{
	if (!player.renderComponent.has_value() || !player.combatComponent.has_value())
	{
		return;
	}

	if (!player.combatComponent->IsAlive())
	{
		player.renderComponent->tint = { 0.28f, 0.30f, 0.34f };
		return;
	}

	const float healthRatio = static_cast<float>(player.combatComponent->health) / static_cast<float>(std::max(1, player.combatComponent->maxHealth));
	const float cooldownRatio = attackCooldownRemaining > 0.0f ? attackCooldownRemaining / attackCooldown : 0.0f;
	player.renderComponent->tint =
	{
		0.55f + (1.0f - cooldownRatio) * 0.35f,
		0.72f + healthRatio * 0.20f,
		0.30f + healthRatio * 0.25f
	};
}

float PlayerController::ComputePlayerYawRadians(const XMFLOAT3& from, const XMFLOAT3& to) noexcept
{
	const float dx = to.x - from.x;
	const float dz = to.z - from.z;
	if (std::fabs(dx) < 0.0001f && std::fabs(dz) < 0.0001f)
	{
		return 0.0f;
	}

	return std::atan2(dx, dz);
}
