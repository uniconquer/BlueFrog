#include "Scene.h"
#include <utility>

SceneObject& Scene::CreateObject(std::string name)
{
	objects.push_back({ std::move(name) });
	return objects.back();
}

void Scene::Clear() noexcept
{
	objects.clear();
}

SceneObject* Scene::FindObject(std::string_view name) noexcept
{
	for (auto& object : objects)
	{
		if (object.name == name)
		{
			return &object;
		}
	}
	return nullptr;
}

const SceneObject* Scene::FindObject(std::string_view name) const noexcept
{
	for (const auto& object : objects)
	{
		if (object.name == name)
		{
			return &object;
		}
	}
	return nullptr;
}

std::vector<SceneObject>& Scene::GetObjects() noexcept
{
	return objects;
}

const std::vector<SceneObject>& Scene::GetObjects() const noexcept
{
	return objects;
}
