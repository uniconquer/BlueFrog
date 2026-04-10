#pragma once

#include "../../Engine/Scene/Scene.h"
#include "EnemyScoutBehavior.h"

class SimpleEnemyController
{
public:
	void Update(Scene& scene, float dt) noexcept;
private:
	static SceneObject* FindEnemy(Scene& scene) noexcept;
private:
	EnemyScoutBehavior enemyBehavior;
};
