#pragma once
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

	bool CanRender() const noexcept
	{
		return enabled && renderComponent.has_value();
	}
};
