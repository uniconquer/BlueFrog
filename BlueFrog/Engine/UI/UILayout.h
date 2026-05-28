#pragma once

#include "HealthBar.h"
#include <DirectXMath.h>

namespace UiLayout
{
	inline constexpr DirectX::XMFLOAT3 PanelTint{ 0.08f, 0.09f, 0.12f };
	inline constexpr DirectX::XMFLOAT3 PlayerFillTint{ 0.18f, 0.84f, 0.36f };
	inline constexpr DirectX::XMFLOAT3 CooldownFillTint{ 0.95f, 0.78f, 0.22f };
	// Heavy slash uses a cooler/violet fill so the two skill slots are
	// distinguishable at a glance — same horizontal bar metaphor as slot 0
	// (no need to invent a new UI primitive yet).
	inline constexpr DirectX::XMFLOAT3 HeavyCooldownFillTint{ 0.75f, 0.42f, 0.90f };
	inline constexpr DirectX::XMFLOAT3 TargetFillTint{ 0.92f, 0.24f, 0.22f };

	inline constexpr float PlayerHealthCenterX = -0.62f;
	inline constexpr float PlayerHealthCenterY = 0.88f;
	inline constexpr float PlayerHealthWidth = 0.48f;
	inline constexpr float PlayerHealthHeight = 0.06f;

	inline constexpr float AttackCooldownCenterY = 0.78f;
	inline constexpr float AttackCooldownHeight = 0.03f;
	// Heavy slash slot sits just below the LMB slot so the two read as a
	// vertical "skill bar" stack near the player HP.
	inline constexpr float HeavyAttackCooldownCenterY = 0.73f;

	inline constexpr float TargetHealthCenterX = 0.00f;
	inline constexpr float TargetHealthCenterY = 0.88f;
	inline constexpr float TargetHealthWidth = 0.42f;
	inline constexpr float TargetHealthHeight = 0.06f;

	inline HealthBar MakePlayerHealthBar(float ratio) noexcept
	{
		return { PlayerHealthCenterX, PlayerHealthCenterY, PlayerHealthWidth, PlayerHealthHeight, ratio, PanelTint, PlayerFillTint };
	}

	inline HealthBar MakeAttackCooldownBar(float ratio) noexcept
	{
		return { PlayerHealthCenterX, AttackCooldownCenterY, PlayerHealthWidth, AttackCooldownHeight, ratio, PanelTint, CooldownFillTint };
	}

	inline HealthBar MakeHeavyAttackCooldownBar(float ratio) noexcept
	{
		return { PlayerHealthCenterX, HeavyAttackCooldownCenterY, PlayerHealthWidth, AttackCooldownHeight, ratio, PanelTint, HeavyCooldownFillTint };
	}

	inline HealthBar MakeTargetHealthBar(float ratio) noexcept
	{
		return { TargetHealthCenterX, TargetHealthCenterY, TargetHealthWidth, TargetHealthHeight, ratio, PanelTint, TargetFillTint };
	}
}
