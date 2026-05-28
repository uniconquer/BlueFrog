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
		playerController.Update(ctx.input, ctx.scene, ctx.camera, ctx.dt, ctx.eventBus, ctx.audio, ctx.damagePopups, ctx.skills);
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
