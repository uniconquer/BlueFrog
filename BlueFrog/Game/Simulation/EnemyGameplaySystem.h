#pragma once

#include "../NPC/SimpleEnemyController.h"

class EnemyGameplaySystem final
{
public:
	void Update(Scene& scene, float dt) noexcept
	{
		enemyController.Update(scene, dt);
	}

private:
	SimpleEnemyController enemyController;
};
