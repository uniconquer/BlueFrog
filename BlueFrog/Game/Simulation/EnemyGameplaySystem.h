#pragma once

#include "../NPC/SimpleEnemyController.h"
#include "SystemContext.h"

class EnemyGameplaySystem final
{
public:
	void Update(const SystemContext& ctx) noexcept
	{
		enemyController.Update(ctx.scene, ctx.dt, ctx.eventBus);
	}

private:
	SimpleEnemyController enemyController;
};
