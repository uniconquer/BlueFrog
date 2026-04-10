#include "SimpleEnemyController.h"
#include "../Simulation/GameplaySceneIds.h"

void SimpleEnemyController::Update(Scene& scene, float dt) noexcept
{
	SceneObject* player = scene.FindObject(GameplaySceneIds::Player);
	SceneObject* enemy = FindEnemy(scene);
	if (player == nullptr || enemy == nullptr)
	{
		return;
	}

	enemyBehavior.Update(scene, *player, *enemy, dt);
}

SceneObject* SimpleEnemyController::FindEnemy(Scene& scene) noexcept
{
	return scene.FindObject(GameplaySceneIds::EnemyScout);
}
