#include "CollisionSystem.h"
#include "AABB.h"

namespace
{
	AABB BuildBounds(const SceneObject& object, const DirectX::XMFLOAT3& position) noexcept
	{
		const auto& collision = *object.collisionComponent;
		return
		{
			{ position.x, position.z },
			collision.halfExtents
		};
	}

	bool IsBlockingCollisionPair(const SceneObject& actor, const SceneObject& other) noexcept
	{
		if (&actor == &other || !other.enabled || !other.collisionComponent.has_value())
		{
			return false;
		}

		if (!other.collisionComponent->blocksMovement)
		{
			return false;
		}

		if (other.combatComponent.has_value() && !other.combatComponent->IsAlive())
		{
			return false;
		}

		return true;
	}
}

void CollisionSystem::MoveAndSlide(SceneObject& actor, const Scene& scene, const DirectX::XMFLOAT3& desiredPosition) noexcept
{
	if (!actor.collisionComponent.has_value())
	{
		actor.transform.position = desiredPosition;
		return;
	}

	const DirectX::XMFLOAT3 originalPosition = actor.transform.position;
	DirectX::XMFLOAT3 resolvedPosition = originalPosition;

	resolvedPosition.x = desiredPosition.x;
	if (CollidesAt(actor, scene, resolvedPosition))
	{
		resolvedPosition.x = originalPosition.x;
	}

	resolvedPosition.z = desiredPosition.z;
	if (CollidesAt(actor, scene, resolvedPosition))
	{
		resolvedPosition.z = originalPosition.z;
	}

	resolvedPosition.y = desiredPosition.y;
	actor.transform.position = resolvedPosition;
}

bool CollisionSystem::CollidesAt(const SceneObject& actor, const Scene& scene, const DirectX::XMFLOAT3& position) noexcept
{
	if (!actor.collisionComponent.has_value())
	{
		return false;
	}

	const AABB actorBounds = BuildBounds(actor, position);
	for (const auto& other : scene.GetObjects())
	{
		if (!IsBlockingCollisionPair(actor, other))
		{
			continue;
		}

		if (actorBounds.Intersects(BuildBounds(other, other.transform.position)))
		{
			return true;
		}
	}

	return false;
}
