#pragma once
#include "../../Core/Window.h"
#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include <DirectXMath.h>

class PlayerController
{
public:
	bool Update(Window& wnd, Scene& scene, TopDownCamera& camera, float dt, bool attackQueued) noexcept;
	float GetAttackCooldownProgress01() const noexcept;
private:
	SceneObject* FindPlayer(Scene& scene) noexcept;
	static DirectX::XMFLOAT3 GetMoveVector(const Window& wnd, const TopDownCamera& camera) noexcept;
	static bool ComputeMouseGroundPoint(const Window& wnd, const TopDownCamera& camera, DirectX::XMFLOAT3& outPoint) noexcept;
	bool TryAttack(Scene& scene, SceneObject& player) noexcept;
	void UpdateTint(SceneObject& player) const noexcept;
	static float ComputePlayerYawRadians(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to) noexcept;
private:
	static constexpr float moveSpeed = 6.5f;
	static constexpr float playerHeight = 1.25f;
	static constexpr float attackRange = 2.4f;
	static constexpr float attackCooldown = 0.45f;
	static constexpr int attackDamage = 1;
	float attackCooldownRemaining = 0.0f;
};
