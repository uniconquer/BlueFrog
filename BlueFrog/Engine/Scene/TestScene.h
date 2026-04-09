#pragma once
#include "SceneObject.h"
#include <vector>

class TestScene
{
public:
	TestScene();

	const std::vector<SceneObject>& GetObjects() const noexcept;
	std::vector<SceneObject>& GetObjects() noexcept;
	void Clear() noexcept;
	SceneObject& AddObject(SceneObject object);
	void BuildDefaultTopDownTestMap();
private:
	std::vector<SceneObject> objects;
};
