#pragma once

#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Scene/CollisionComponent.h"
#include "../../Engine/Scene/CombatComponent.h"
#include "../../Engine/Scene/RenderComponent.h"
#include "../../Engine/Scene/SceneObject.h"
#include "GameplaySceneIds.h"
#include <DirectXMath.h>
#include <string>

class GameplayArenaBuilder final
{
public:
	static void Build(Scene& scene, TopDownCamera& camera) noexcept
	{
		BuildArenaGeometry(scene, camera);
	}
private:
	static void BuildArenaGeometry(Scene& scene, TopDownCamera& camera) noexcept
	{
		using DirectX::XMFLOAT2;
		using DirectX::XMFLOAT3;

		scene.Clear();

		const auto createRenderable = [&](std::string_view name, const XMFLOAT3& position, const XMFLOAT3& scale, RenderMeshType meshType, const XMFLOAT3& tint) -> SceneObject&
		{
			auto& object = scene.CreateObject(std::string(name));
			object.transform.position = position;
			object.transform.scale = scale;
			object.renderComponent = RenderComponent{ meshType, tint };
			return object;
		};

		const auto attachBlocker = [](SceneObject& object, const XMFLOAT2& halfExtents)
		{
			object.collisionComponent = CollisionComponent{ halfExtents, true };
		};

		createRenderable("Ground", { 0.0f, 0.0f, 0.0f }, { 18.0f, 1.0f, 18.0f }, RenderMeshType::Plane, { 0.92f, 1.0f, 0.92f });
		auto& shrine = createRenderable("ShrineCore", { 0.0f, 1.25f, 0.0f }, { 1.35f, 1.35f, 1.35f }, RenderMeshType::Cube, { 1.0f, 0.94f, 0.80f });
		attachBlocker(shrine, { 1.35f, 1.35f });

		auto& northWall = createRenderable("NorthWall", { 0.0f, 1.0f, 6.5f }, { 6.0f, 1.0f, 0.7f }, RenderMeshType::Cube, { 0.86f, 0.91f, 1.0f });
		auto& southWall = createRenderable("SouthWall", { 0.0f, 1.0f, -6.5f }, { 6.0f, 1.0f, 0.7f }, RenderMeshType::Cube, { 0.86f, 0.91f, 1.0f });
		auto& eastWall = createRenderable("EastWall", { 6.5f, 1.0f, 0.0f }, { 0.7f, 1.0f, 6.0f }, RenderMeshType::Cube, { 0.86f, 0.91f, 1.0f });
		auto& westWall = createRenderable("WestWall", { -6.5f, 1.0f, 0.0f }, { 0.7f, 1.0f, 6.0f }, RenderMeshType::Cube, { 0.86f, 0.91f, 1.0f });
		attachBlocker(northWall, { 6.0f, 0.7f });
		attachBlocker(southWall, { 6.0f, 0.7f });
		attachBlocker(eastWall, { 0.7f, 6.0f });
		attachBlocker(westWall, { 0.7f, 6.0f });

		auto& pillarA = createRenderable("PillarA", { -3.5f, 0.8f, -2.5f }, { 0.8f, 0.8f, 0.8f }, RenderMeshType::Cube, { 0.95f, 0.86f, 1.0f });
		auto& pillarB = createRenderable("PillarB", { 3.5f, 0.8f, 2.5f }, { 0.8f, 0.8f, 0.8f }, RenderMeshType::Cube, { 1.0f, 0.88f, 0.84f });
		attachBlocker(pillarA, { 0.8f, 0.8f });
		attachBlocker(pillarB, { 0.8f, 0.8f });

		auto& player = createRenderable(GameplaySceneIds::Player, { -4.0f, 1.25f, 0.0f }, { 0.7f, 1.25f, 0.7f }, RenderMeshType::Cube, { 0.82f, 1.0f, 0.55f });
		player.transform.rotation = { 0.0f, DirectX::XM_PIDIV2, 0.0f };
		player.collisionComponent = CollisionComponent{ { 0.45f, 0.45f }, true };
		player.combatComponent = CombatComponent{ CombatFaction::Player, 5, 5 };

		auto& enemy = createRenderable(GameplaySceneIds::EnemyScout, { 3.8f, 1.0f, -3.8f }, { 0.75f, 1.0f, 0.75f }, RenderMeshType::Cube, { 0.92f, 0.36f, 0.36f });
		enemy.collisionComponent = CollisionComponent{ { 0.5f, 0.5f }, true };
		enemy.combatComponent = CombatComponent{ CombatFaction::Enemy, 3, 3 };

		camera.SetTarget(player.transform.position);
	}
};
