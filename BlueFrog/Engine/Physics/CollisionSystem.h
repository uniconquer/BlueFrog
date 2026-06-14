#pragma once
#include "../Scene/Scene.h"

#include <optional>

class CollisionSystem
{
public:
	// Vertical body model for the 2.5D layer (see CollisionComponent):
	// actors are kActorHeight tall, and ignore blockers whose top sits less
	// than kStepHeight above their feet — that sliver is "stepped onto"
	// instead, because FloorHeightAt also reports such tops as floor.
	static constexpr float kActorHeight = 1.7f;
	static constexpr float kStepHeight  = 0.35f;
	// Mantle (Jump V2): the tallest ledge an airborne actor can grab and
	// pull up onto, measured above its feet at the moment of contact. With
	// the ~1.3m jump apex this reaches a single-storey eave (~3.3m) from
	// the ground.
	static constexpr float kMantleReach = 2.0f;

	static void MoveAndSlide(SceneObject& actor, const Scene& scene, const DirectX::XMFLOAT3& desiredPosition) noexcept;

	// Highest walkable surface under the actor's disc at (x, z) that is no
	// higher than the actor's feet + kStepHeight: the max topY among
	// blocking boxes overlapped in XZ, or 0 (the world ground plane).
	// `actorY` is the actor's current feet height.
	static float FloorHeightAt(const SceneObject& actor, const Scene& scene, float x, float z, float actorY) noexcept;

	// Mantle target search: a probe one body-radius ahead in (dirX,dirZ)
	// finds the highest blocker top within the mantle band
	// (feet+kStepHeight .. feet+kMantleReach) whose surface has standing
	// room. Returns the pull-up position (forward XZ + ledge top), else
	// nullopt. dir must be a normalized horizontal direction.
	static std::optional<DirectX::XMFLOAT3> FindMantleTarget(const SceneObject& actor, const Scene& scene, float dirX, float dirZ, float actorY) noexcept;
private:
	static bool CollidesAt(const SceneObject& actor, const Scene& scene, const DirectX::XMFLOAT3& position) noexcept;
};
