#pragma once

#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"
#include "../Player/PlayerController.h"
#include "../Simulation/GameplaySceneIds.h"

class HudPresenter final
{
public:
	[[nodiscard]] static HudState Build(const Scene& scene, const PlayerController& playerController) noexcept
	{
		HudState hudState{};

		const SceneObject* player = scene.FindObject(GameplaySceneIds::Player);
		if (player != nullptr && player->combatComponent.has_value())
		{
			hudState.playerHealth.current = static_cast<float>(player->combatComponent->health);
			hudState.playerHealth.max = static_cast<float>(player->combatComponent->maxHealth);
		}

		const SceneObject* enemy = scene.FindObject(GameplaySceneIds::EnemyScout);
		if (enemy != nullptr && enemy->combatComponent.has_value() && enemy->combatComponent->IsAlive())
		{
			hudState.hasTarget = true;
			hudState.targetHealth.current = static_cast<float>(enemy->combatComponent->health);
			hudState.targetHealth.max = static_cast<float>(enemy->combatComponent->maxHealth);
		}

		hudState.attackCooldown01 = playerController.GetAttackCooldownProgress01();
		hudState.objectiveText = hudState.hasTarget ? L"Defeat the scout" : L"Arena cleared";
		return hudState;
	}
};
