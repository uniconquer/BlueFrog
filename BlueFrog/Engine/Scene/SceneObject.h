#pragma once
#include "AnimationStateComponent.h"
#include "CollisionComponent.h"
#include "CombatComponent.h"
#include "EnemyBehaviorComponent.h"
#include "RenderComponent.h"
#include "Transform.h"
#include "TriggerComponent.h"
#include <optional>
#include <string>

struct SceneObject
{
	std::string name;
	bool enabled = true;
	Transform transform;
	std::optional<RenderComponent>   renderComponent;
	std::optional<CollisionComponent> collisionComponent;
	std::optional<CombatComponent>   combatComponent;
	std::optional<TriggerComponent>  triggerComponent;
	std::optional<EnemyBehaviorComponent> enemyBehaviorComponent;
	std::optional<AnimationStateComponent> animationStateComponent;

	bool CanRender() const noexcept
	{
		return enabled && renderComponent.has_value();
	}
};
