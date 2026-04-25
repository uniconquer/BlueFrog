#pragma once

#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"
#include "../Hud/HudPresenter.h"
#include "../Player/PlayerController.h"
#include "SystemContext.h"

class PlayerGameplaySystem final
{
public:
	void Update(const SystemContext& ctx) noexcept
	{
		playerController.Update(ctx.input, ctx.scene, ctx.camera, ctx.dt, ctx.eventBus);
	}

	[[nodiscard]] HudState BuildHudState(const Scene& scene) const noexcept
	{
		return HudPresenter::Build(scene, playerController);
	}
private:
	PlayerController playerController;
};
