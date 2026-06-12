#pragma once

#include "../../Engine/Render/ParticleSystem.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"
#include "../Hud/HudPresenter.h"
#include "../Player/PlayerController.h"
#include "GameplaySceneIds.h"
#include "SystemContext.h"

class PlayerGameplaySystem final
{
public:
	void Update(const SystemContext& ctx) noexcept
	{
		playerController.Update(ctx.input, ctx.scene, ctx.camera, ctx.dt, ctx.eventBus, ctx.audio, ctx.damagePopups, ctx.skills);

		// Hard-landing feedback (Jump V1): a >=3.5m fall just ended — kick
		// up a dust puff at the feet to sell the impact alongside the
		// JumpLand pose + input stun the controller applies.
		if (playerController.ConsumeHardLanded() && ctx.particles != nullptr)
		{
			if (const SceneObject* p = ctx.scene.FindObject(GameplaySceneIds::Player))
			{
				DirectX::XMFLOAT3 pos = p->transform.position;
				pos.y += 0.12f;
				ctx.particles->Burst(pos, 12, 2.4f, 0.5f,
					{ 0.66f, 0.58f, 0.46f, 0.85f },
					{ 0.66f, 0.58f, 0.46f, 0.0f },
					0.32f);
			}
		}
	}

	[[nodiscard]] HudState BuildHudState(const Scene& scene) const noexcept
	{
		return HudPresenter::Build(scene, playerController);
	}

	// FLApp uses this to hand the controller a mount on E-press, and to
	// query mounted state when routing input. Exposed via a getter rather
	// than making the controller a public member so external callers
	// can't reach into the rest of its state.
	[[nodiscard]] PlayerController& GetController() noexcept { return playerController; }
	[[nodiscard]] const PlayerController& GetController() const noexcept { return playerController; }
private:
	PlayerController playerController;
};
