#include "NpcFacingSystem.h"

#include "../Simulation/GameplaySceneIds.h"

#include <cmath>

namespace NpcFacingSystem
{
	void Tick(Scene& scene) noexcept
	{
		const SceneObject* player = scene.FindObject(GameplaySceneIds::Player);
		if (player == nullptr)
		{
			return;
		}

		const float noticeRangeSq = NoticeRange * NoticeRange;
		for (SceneObject& obj : scene.GetObjects())
		{
			if (&obj == player) continue;
			if (!obj.npcComponent.has_value()) continue;

			const float dx = player->transform.position.x - obj.transform.position.x;
			const float dz = player->transform.position.z - obj.transform.position.z;
			const float distSq = dx * dx + dz * dz;
			if (distSq > noticeRangeSq) continue;
			if (distSq < 0.0001f) continue; // player on top of NPC: leave rotation alone

			// Same yaw convention as PlayerAimSystem: atan2(dx, dz) so a
			// positive Z direction is yaw 0, X+ is yaw +90°. Matches the
			// engine's left-handed top-down setup.
			obj.transform.rotation.y = std::atan2(dx, dz);
		}
	}
}
