#pragma once

#include "../Scene/Scene.h"
#include "../Scene/SceneObject.h"

// Advances per-instance animation clipTime once per tick. Header-only:
// implementation is small, has no out-of-line state, and being inline makes
// the loop trivially auto-vectorize when scenes start carrying many animated
// objects.
//
// Stage 4a (this revision):
//   - Tick clipTime by dt * playSpeed for every SceneObject that carries
//     an AnimationStateComponent.
//   - If the component requests looping (the default), we don't bother
//     wrapping clipTime here — the renderer applies fmod(clipTime, duration)
//     when it samples. Wrapping at this layer would require knowing
//     `duration`, which lives on the mesh side; cleaner to leave clipTime
//     monotonic and let the sampler handle the wrap.
//   - When `looping == false` the system clamps clipTime so it stops
//     accumulating after a generous upper bound (1e6 seconds). The render
//     sampler pins to the last keyframe regardless once clipTime exceeds
//     the clip's duration; the clamp here only prevents float overflow on
//     pathological play durations.
//
// Stage 4b will plug a controller (state machine) on top: it inspects
// gameplay state each tick and *writes* clipName / playSpeed / clipTime
// here before this system runs.
namespace AnimationSystem
{
	inline void Tick(Scene& scene, float dt) noexcept
	{
		for (SceneObject& obj : scene.GetObjects())
		{
			if (!obj.animationStateComponent.has_value()) continue;
			auto& a = obj.animationStateComponent.value();
			a.clipTime += dt * a.playSpeed;
			if (!a.looping)
			{
				constexpr float kMaxClipTime = 1.0e6f;
				if (a.clipTime > kMaxClipTime) a.clipTime = kMaxClipTime;
				if (a.clipTime < 0.0f) a.clipTime = 0.0f;
			}
		}
	}
}
