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

	// Drive every alive enemy-faction combatant in the scene. Behavior is
	// stateless and reads its per-instance cooldown off the SceneObject's
	// CombatComponent, so a single instance can fan out across N enemies
	// without per-name plumbing.
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

		enemyBehavior.Update(scene, *player, obj, dt, bus);
	}
}
