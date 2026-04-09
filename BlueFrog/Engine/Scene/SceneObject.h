#pragma once
#include "RenderComponent.h"
#include "Transform.h"
#include <optional>
#include <string>
#include <utility>

class SceneObject
{
public:
	SceneObject() = default;
	explicit SceneObject(std::string nameIn)
		:
		name(std::move(nameIn))
	{
	}

	const std::string& GetName() const noexcept
	{
		return name;
	}

	void SetName(std::string nameIn)
	{
		name = std::move(nameIn);
	}

	bool IsEnabled() const noexcept
	{
		return enabled;
	}

	void SetEnabled(bool value) noexcept
	{
		enabled = value;
	}

	Transform& GetTransform() noexcept
	{
		return transform;
	}

	const Transform& GetTransform() const noexcept
	{
		return transform;
	}

	bool HasRenderComponent() const noexcept
	{
		return renderComponent.has_value();
	}

	RenderComponent& EmplaceRenderComponent(const RenderComponent& component) noexcept
	{
		renderComponent = component;
		return *renderComponent;
	}

	RenderComponent* TryGetRenderComponent() noexcept
	{
		return renderComponent ? &*renderComponent : nullptr;
	}

	const RenderComponent* TryGetRenderComponent() const noexcept
	{
		return renderComponent ? &*renderComponent : nullptr;
	}

	void ClearRenderComponent() noexcept
	{
		renderComponent.reset();
	}
private:
	std::string name = "SceneObject";
	bool enabled = true;
	Transform transform;
	std::optional<RenderComponent> renderComponent;
};
