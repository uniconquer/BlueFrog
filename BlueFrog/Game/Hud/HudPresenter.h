#pragma once

#include "../../Engine/Scene/CombatComponent.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/HudState.h"
#include "../Player/PlayerController.h"
#include "../Simulation/GameplaySceneIds.h"
#include <cmath>

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
			hudState.playerDefeated = !player->combatComponent->IsAlive();
		}

		// Pick the nearest alive enemy-faction combatant as the HUD target.
		// Walking the scene each frame matches what SimpleEnemyController
		// does for AI; consistent target selection across systems means a
		// scene with multiple enemies always shows the threat the player
		// most needs to read.
		if (player != nullptr)
		{
			const SceneObject* nearest = nullptr;
			float nearestDistSq = 0.0f;
			for (const SceneObject& obj : scene.GetObjects())
			{
				if (&obj == player) continue;
				if (!obj.combatComponent.has_value()) continue;
				if (obj.combatComponent->faction != CombatFaction::Enemy) continue;
				if (!obj.combatComponent->IsAlive()) continue;

				const float dx = obj.transform.position.x - player->transform.position.x;
				const float dz = obj.transform.position.z - player->transform.position.z;
				const float distSq = dx * dx + dz * dz;
				if (nearest == nullptr || distSq < nearestDistSq)
				{
					nearest = &obj;
					nearestDistSq = distSq;
				}
			}
			if (nearest != nullptr)
			{
				hudState.hasTarget = true;
				hudState.targetHealth.current = static_cast<float>(nearest->combatComponent->health);
				hudState.targetHealth.max = static_cast<float>(nearest->combatComponent->maxHealth);
			}
		}

		hudState.attackCooldown01 = playerController.GetAttackCooldownProgress01();
		// objectiveText is injected by GameplaySimulation after BuildHudState so
		// the HUD stays data-driven off ObjectiveSystem rather than deriving
		// text from the scene's enemy roster.
		return hudState;
	}
};
