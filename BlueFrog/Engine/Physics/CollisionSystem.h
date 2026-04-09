#pragma once
#include "../Scene/Scene.h"

class CollisionSystem
{
public:
	static void MoveAndSlide(SceneObject& actor, const Scene& scene, const DirectX::XMFLOAT3& desiredPosition) noexcept;
private:
	static bool CollidesAt(const SceneObject& actor, const Scene& scene, const DirectX::XMFLOAT3& position) noexcept;
};
