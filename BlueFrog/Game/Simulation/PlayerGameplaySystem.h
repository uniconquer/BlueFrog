#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"
#include "GameplayInput.h"
#include "../Hud/HudPresenter.h"
#include "../Player/PlayerController.h"

class PlayerGameplaySystem final
{
public:
	void Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt) noexcept
	{
		playerController.Update(input, scene, camera, dt);
	}

	[[nodiscard]] HudState BuildHudState(const Scene& scene) const noexcept
	{
		return HudPresenter::Build(scene, playerController);
	}
private:
	PlayerController playerController;
};
