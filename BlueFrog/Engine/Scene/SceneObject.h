#pragma once
#include "CollisionComponent.h"
#include "CombatComponent.h"
#include "RenderComponent.h"
#include "Transform.h"
#include <optional>
#include <string>

struct SceneObject
{
	std::string name;
	bool enabled = true;
	Transform transform;
	std::optional<RenderComponent> renderComponent;
	std::optional<CollisionComponent> collisionComponent;
	std::optional<CombatComponent> combatComponent;

	bool CanRender() const noexcept
	{
		return enabled && renderComponent.has_value();
	}
};
