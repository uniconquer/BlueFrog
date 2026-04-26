#include "SimpleEnemyController.h"

#include "../../Engine/Scene/CombatComponent.h"
#include "../Simulation/GameplaySceneIds.h"

void SimpleEnemyController::Update(Scene& scene, float dt, EventBus& bus) noexcept
{
	SceneObject* player = scene.FindObject(GameplaySceneIds::Player);
	if (player == nullptr)
	{
		return;
	}

	for (SceneObject& obj : scene.GetObjects())
	{
		if (&obj == player)
		{
			continue;
		}
		if (!obj.combatComponent.has_value())
		{
			continue;
		}
		if (obj.combatComponent->faction != CombatFaction::Enemy)
		{
			continue;
		}

		// Dispatch on the EnemyBehaviorComponent's type. Absent component =>
		// scout (matches existing prefabs that never opt into a behavior tag).
		// SceneLoader's allow-list ensures any "type" we see here is one of
		// the cases below — typoed types fail at startup with a path-prefixed
		// error, never reach the controller.
		const std::string& type = obj.enemyBehaviorComponent.has_value()
			? obj.enemyBehaviorComponent->type
			: std::string("scout");

		if (type == "archer")
		{
			archerBehavior.Update(scene, *player, obj, dt, bus);
		}
		else
		{
			scoutBehavior.Update(scene, *player, obj, dt, bus);
		}
	}
}
