#pragma once
#include "SceneObject.h"
#include <string_view>
#include <vector>

class Scene
{
public:
	SceneObject& CreateObject(std::string name);
	void Clear() noexcept;
	SceneObject* FindObject(std::string_view name) noexcept;
	const SceneObject* FindObject(std::string_view name) const noexcept;
	std::vector<SceneObject>& GetObjects() noexcept;
	const std::vector<SceneObject>& GetObjects() const noexcept;
private:
	std::vector<SceneObject> objects;
};
