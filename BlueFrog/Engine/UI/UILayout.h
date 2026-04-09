#pragma once

#include "HealthBar.h"
#include <DirectXMath.h>

namespace UiLayout
{
	inline constexpr DirectX::XMFLOAT3 PanelTint{ 0.08f, 0.09f, 0.12f };
	inline constexpr DirectX::XMFLOAT3 PlayerFillTint{ 0.18f, 0.84f, 0.36f };
	inline constexpr DirectX::XMFLOAT3 CooldownFillTint{ 0.95f, 0.78f, 0.22f };
	inline constexpr DirectX::XMFLOAT3 TargetFillTint{ 0.92f, 0.24f, 0.22f };
	inline constexpr DirectX::XMFLOAT3 CrosshairTint{ 0.95f, 0.98f, 1.0f };

	inline constexpr float PlayerHealthCenterX = -0.62f;
	inline constexpr float PlayerHealthCenterY = 0.88f;
	inline constexpr float PlayerHealthWidth = 0.48f;
	inline constexpr float PlayerHealthHeight = 0.06f;

	inline constexpr float AttackCooldownCenterY = 0.78f;
	inline constexpr float AttackCooldownHeight = 0.03f;

	inline constexpr float TargetHealthCenterX = 0.00f;
	inline constexpr float TargetHealthCenterY = 0.88f;
	inline constexpr float TargetHealthWidth = 0.42f;
	inline constexpr float TargetHealthHeight = 0.06f;

	inline constexpr float CrosshairCenterX = 0.0f;
	inline constexpr float CrosshairCenterY = 0.0f;
	inline constexpr float CrosshairLength = 0.08f;
	inline constexpr float CrosshairThickness = 0.006f;
	inline constexpr float CrosshairGap = 0.014f;

	inline HealthBar MakePlayerHealthBar(float ratio) noexcept
	{
		return { PlayerHealthCenterX, PlayerHealthCenterY, PlayerHealthWidth, PlayerHealthHeight, ratio, PanelTint, PlayerFillTint };
	}

	inline HealthBar MakeAttackCooldownBar(float ratio) noexcept
	{
		return { PlayerHealthCenterX, AttackCooldownCenterY, PlayerHealthWidth, AttackCooldownHeight, ratio, PanelTint, CooldownFillTint };
	}

	inline HealthBar MakeTargetHealthBar(float ratio) noexcept
	{
		return { TargetHealthCenterX, TargetHealthCenterY, TargetHealthWidth, TargetHealthHeight, ratio, PanelTint, TargetFillTint };
	}
}
