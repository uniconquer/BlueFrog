#include "TestScene.h"
#include <utility>

TestScene::TestScene()
{
	BuildDefaultTopDownTestMap();
}

const std::vector<SceneObject>& TestScene::GetObjects() const noexcept
{
	return objects;
}

std::vector<SceneObject>& TestScene::GetObjects() noexcept
{
	return objects;
}

void TestScene::Clear() noexcept
{
	objects.clear();
}

SceneObject& TestScene::AddObject(SceneObject object)
{
	objects.push_back(std::move(object));
	return objects.back();
}

void TestScene::BuildDefaultTopDownTestMap()
{
	objects.clear();

	{
		SceneObject ground("Ground");
		ground.GetTransform().scale = { 18.0f, 1.0f, 18.0f };
		ground.EmplaceRenderComponent({ MeshType::Plane, { 1.0f, 1.0f, 1.0f } });
		AddObject(std::move(ground));
	}

	{
		SceneObject central("CentralCube");
		central.GetTransform().position = { 0.0f, 1.25f, 0.0f };
		central.GetTransform().scale = { 1.35f, 1.35f, 1.35f };
		central.GetTransform().rotation = { 0.0f, 0.0f, 0.0f };
		central.EmplaceRenderComponent({ MeshType::Cube, { 1.0f, 1.0f, 1.0f } });
		AddObject(std::move(central));
	}

	{
		SceneObject northWall("NorthWall");
		northWall.GetTransform().position = { 0.0f, 1.0f, 6.5f };
		northWall.GetTransform().scale = { 6.0f, 1.0f, 0.7f };
		northWall.EmplaceRenderComponent({ MeshType::Cube, { 1.0f, 1.0f, 1.0f } });
		AddObject(std::move(northWall));
	}

	{
		SceneObject southWall("SouthWall");
		southWall.GetTransform().position = { 0.0f, 1.0f, -6.5f };
		southWall.GetTransform().scale = { 6.0f, 1.0f, 0.7f };
		southWall.EmplaceRenderComponent({ MeshType::Cube, { 1.0f, 1.0f, 1.0f } });
		AddObject(std::move(southWall));
	}

	{
		SceneObject eastWall("EastWall");
		eastWall.GetTransform().position = { 6.5f, 1.0f, 0.0f };
		eastWall.GetTransform().scale = { 0.7f, 1.0f, 6.0f };
		eastWall.EmplaceRenderComponent({ MeshType::Cube, { 1.0f, 1.0f, 1.0f } });
		AddObject(std::move(eastWall));
	}

	{
		SceneObject westWall("WestWall");
		westWall.GetTransform().position = { -6.5f, 1.0f, 0.0f };
		westWall.GetTransform().scale = { 0.7f, 1.0f, 6.0f };
		westWall.EmplaceRenderComponent({ MeshType::Cube, { 1.0f, 1.0f, 1.0f } });
		AddObject(std::move(westWall));
	}

	{
		SceneObject pillarA("PillarA");
		pillarA.GetTransform().position = { -3.5f, 0.8f, -2.5f };
		pillarA.GetTransform().scale = { 0.8f, 0.8f, 0.8f };
		pillarA.EmplaceRenderComponent({ MeshType::Cube, { 1.0f, 1.0f, 1.0f } });
		AddObject(std::move(pillarA));
	}

	{
		SceneObject pillarB("PillarB");
		pillarB.GetTransform().position = { 3.5f, 0.8f, 2.5f };
		pillarB.GetTransform().scale = { 0.8f, 0.8f, 0.8f };
		pillarB.EmplaceRenderComponent({ MeshType::Cube, { 1.0f, 1.0f, 1.0f } });
		AddObject(std::move(pillarB));
	}
}
