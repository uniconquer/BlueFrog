#include "CollisionSystem.h"

#include <algorithm>
#include <cmath>

namespace
{
	// Circle (the moving actor, approximated as a disc) vs an oriented box
	// (the blocker, an OBB built from halfExtents + the blocker's yaw). Tests
	// in the box's local frame so blockers collide exactly where they sit when
	// rotated — a long thin wall stays long+thin at any angle. Our yaw
	// convention maps local->world as (lx*c + lz*s, -lx*s + lz*c), so the
	// inverse (world->local) is (dx*c - dz*s, dx*s + dz*c).
	bool CircleVsObbXZ(float cx, float cz, float radius,
		const SceneObject& box, const DirectX::XMFLOAT3& boxPos) noexcept
	{
		const auto& bc = *box.collisionComponent;
		const float yaw = box.transform.rotation.y;
		const float c = std::cos(yaw);
		const float s = std::sin(yaw);
		// Collision tracks the object's scale (scatter jitters nature scale,
		// some prefabs shrink NPCs, etc.) so the box matches the scaled mesh.
		const float sx = box.transform.scale.x;
		const float sz = box.transform.scale.z;
		const float offX = bc.offset.x * sx;
		const float offZ = bc.offset.y * sz;
		const float hX = bc.halfExtents.x * sx;
		const float hZ = bc.halfExtents.y * sz;
		// Box center = object position + the local offset rotated into world
		// (same local->world convention as the corners below).
		const float wcx = boxPos.x + offX * c + offZ * s;
		const float wcz = boxPos.z - offX * s + offZ * c;
		const float dx = cx - wcx;
		const float dz = cz - wcz;
		const float lx = dx * c - dz * s;
		const float lz = dx * s + dz * c;
		const float clampedX = std::clamp(lx, -hX, hX);
		const float clampedZ = std::clamp(lz, -hZ, hZ);
		const float ddx = lx - clampedX;
		const float ddz = lz - clampedZ;
		return (ddx * ddx + ddz * ddz) < (radius * radius);
	}

	// World-space vertical span of a blocker's box: local baseY/topY scaled
	// by the object's Y scale, offset by its position. The 1e9 legacy
	// default stays effectively infinite through any sane scale.
	void BoxWorldYRange(const SceneObject& box, float& outBase, float& outTop) noexcept
	{
		const auto& bc = *box.collisionComponent;
		const float sy = box.transform.scale.y;
		outBase = box.transform.position.y + bc.baseY * sy;
		outTop  = box.transform.position.y + (std::min)(bc.topY, 1.0e9f) * sy;
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

	// Treat the moving actor as a disc (radius = its larger half-extent); test
	// it against every blocker as an oriented box. Disc keeps the X-then-Z
	// slide in MoveAndSlide simple and is a fine fit for the roughly-round
	// actors (player, enemies, mount).
	const float radius = (std::max)(actor.collisionComponent->halfExtents.x,
	                                actor.collisionComponent->halfExtents.y);
	// 2.5D: the actor's solid body spans [feet + kStepHeight, feet +
	// kActorHeight]. Starting at knee height means boxes whose top pokes
	// less than a step above the floor never block — the floor query
	// "steps onto" them instead — and boxes entirely above the head
	// (e.g. a bridge deck while walking under it) are passed beneath.
	const float bodyLo = position.y + kStepHeight;
	const float bodyHi = position.y + kActorHeight;
	for (const auto& other : scene.GetObjects())
	{
		if (!IsBlockingCollisionPair(actor, other))
		{
			continue;
		}
		float boxBase = 0.0f, boxTop = 0.0f;
		BoxWorldYRange(other, boxBase, boxTop);
		if (bodyLo >= boxTop || bodyHi <= boxBase)
		{
			continue; // vertically clear of this blocker
		}
		if (CircleVsObbXZ(position.x, position.z, radius, other, other.transform.position))
		{
			return true;
		}
	}

	return false;
}

float CollisionSystem::FloorHeightAt(const SceneObject& actor, const Scene& scene, float x, float z, float actorY) noexcept
{
	// The world ground plane is the universal fallback floor.
	float best = 0.0f;
	if (!actor.collisionComponent.has_value())
	{
		return best;
	}

	// Slightly smaller probe than the blocking disc so the actor can't
	// "stand" on a surface they barely clip at the rim.
	const float radius = 0.8f * (std::max)(actor.collisionComponent->halfExtents.x,
	                                       actor.collisionComponent->halfExtents.y);
	const float reach = actorY + kStepHeight; // highest top we can stand on / step up to
	for (const auto& other : scene.GetObjects())
	{
		if (!IsBlockingCollisionPair(actor, other))
		{
			continue;
		}
		float boxBase = 0.0f, boxTop = 0.0f;
		BoxWorldYRange(other, boxBase, boxTop);
		if (boxTop > reach || boxTop <= best)
		{
			continue; // above our step reach, or not an improvement
		}
		if (CircleVsObbXZ(x, z, radius, other, other.transform.position))
		{
			best = boxTop;
		}
	}
	return best;
}
