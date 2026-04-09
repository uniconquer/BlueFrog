#include "PlayerController.h"
#include "../../Engine/Physics/CollisionSystem.h"
#include "../Combat/CombatSystem.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	XMFLOAT3 MakeMoveVector(bool left, bool right, bool forward, bool back, const TopDownCamera& camera) noexcept
	{
		float x = 0.0f;
		float z = 0.0f;

		if (left)
		{
			x -= 1.0f;
		}
		if (right)
		{
			x += 1.0f;
		}
		if (forward)
		{
			z += 1.0f;
		}
		if (back)
		{
			z -= 1.0f;
		}

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

bool PlayerController::Update(Window& wnd, Scene& scene, TopDownCamera& camera, float dt, bool attackQueued) noexcept
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
		camera.SetTarget(player->transform.position);
		return true;
	}

	const XMFLOAT3 move = GetMoveVector(wnd, camera);
	XMFLOAT3 desiredPosition = player->transform.position;
	desiredPosition.x += move.x * moveSpeed * dt;
	desiredPosition.z += move.z * moveSpeed * dt;
	desiredPosition.y = playerHeight;
	ClampToTestMapBounds(desiredPosition);
	CollisionSystem::MoveAndSlide(*player, scene, desiredPosition);

	XMFLOAT3 mouseGroundPoint = player->transform.position;
	if (ComputeMouseGroundPoint(wnd, camera, mouseGroundPoint))
	{
		player->transform.rotation.y = ComputePlayerYawRadians(player->transform.position, mouseGroundPoint);
	}

	if (attackQueued && attackCooldownRemaining <= 0.0f)
	{
		TryAttack(scene, *player);
		attackCooldownRemaining = attackCooldown;
	}

	UpdateTint(*player);
	camera.SetTarget(player->transform.position);
	return true;
}

SceneObject* PlayerController::FindPlayer(Scene& scene) noexcept
{
	return scene.FindObject("Player");
}

XMFLOAT3 PlayerController::GetMoveVector(const Window& wnd, const TopDownCamera& camera) noexcept
{
	return MakeMoveVector(
		wnd.kbd.KeyIsPressed('A'),
		wnd.kbd.KeyIsPressed('D'),
		wnd.kbd.KeyIsPressed('W'),
		wnd.kbd.KeyIsPressed('S'),
		camera);
}

bool PlayerController::ComputeMouseGroundPoint(const Window& wnd, const TopDownCamera& camera, XMFLOAT3& outPoint) noexcept
{
	if (!wnd.mouse.IsInWindow())
	{
		return false;
	}

	const auto mousePos = wnd.mouse.GetPos();
	const float width = static_cast<float>(std::max(1, wnd.GetWidth()));
	const float height = static_cast<float>(std::max(1, wnd.GetHeight()));
	const float ndcX = (static_cast<float>(mousePos.first) / width) * 2.0f - 1.0f;
	const float ndcY = 1.0f - (static_cast<float>(mousePos.second) / height) * 2.0f;

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

void PlayerController::ClampToTestMapBounds(XMFLOAT3& position) noexcept
{
	position.x = std::clamp(position.x, minX, maxX);
	position.z = std::clamp(position.z, minZ, maxZ);
	position.y = playerHeight;
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
