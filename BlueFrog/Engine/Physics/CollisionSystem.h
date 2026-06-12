#pragma once
#include "../Scene/Scene.h"

class CollisionSystem
{
public:
	// Vertical body model for the 2.5D layer (see CollisionComponent):
	// actors are kActorHeight tall, and ignore blockers whose top sits less
	// than kStepHeight above their feet — that sliver is "stepped onto"
	// instead, because FloorHeightAt also reports such tops as floor.
	static constexpr float kActorHeight = 1.7f;
	static constexpr float kStepHeight  = 0.35f;

	static void MoveAndSlide(SceneObject& actor, const Scene& scene, const DirectX::XMFLOAT3& desiredPosition) noexcept;

	// Highest walkable surface under the actor's disc at (x, z) that is no
	// higher than the actor's feet + kStepHeight: the max topY among
	// blocking boxes overlapped in XZ, or 0 (the world ground plane).
	// `actorY` is the actor's current feet height.
	static float FloorHeightAt(const SceneObject& actor, const Scene& scene, float x, float z, float actorY) noexcept;
private:
	static bool CollidesAt(const SceneObject& actor, const Scene& scene, const DirectX::XMFLOAT3& position) noexcept;
};
