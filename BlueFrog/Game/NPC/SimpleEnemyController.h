#pragma once

#include "../../Engine/Events/EventBus.h"
#include "../../Engine/Scene/Scene.h"
#include "EnemyScoutBehavior.h"

class SimpleEnemyController
{
public:
	void Update(Scene& scene, float dt, EventBus& bus) noexcept;
private:
	static SceneObject* FindEnemy(Scene& scene) noexcept;
private:
	EnemyScoutBehavior enemyBehavior;
};
