#pragma once

#include "../../Engine/Events/EventBus.h"
#include "../NPC/SimpleEnemyController.h"

class EnemyGameplaySystem final
{
public:
	void Update(Scene& scene, float dt, EventBus& bus) noexcept
	{
		enemyController.Update(scene, dt, bus);
	}

private:
	SimpleEnemyController enemyController;
};
