#pragma once

#include <DirectXMath.h>
#include <vector>

// Tiny CPU-side particle system. One vector of POD Particle structs;
// gameplay code calls Burst(pos, ...) to emit, and FLApp ticks ages
// + erases expired entries each frame. Rendering is handled by
// ParticleRenderer reading the same vector.
//
// Why CPU and not GPU: the game's particle count is in the hundreds
// at most (every hit emits 5-8). The cost of one std::vector pass +
// a few dozen extra draw calls per frame is invisible compared to
// the existing skinned mesh pass. GPU compute / instancing would be
// premature; if the count ever climbs past a few thousand we can
// swap the storage in place without touching the gameplay-facing
// Burst API.

struct Particle
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 velocity;
	float             age    = 0.0f;
	float             maxAge = 1.0f;
	float             size   = 0.25f;   // half-extent on XZ
	DirectX::XMFLOAT4 colorStart = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 colorEnd   = { 1.0f, 1.0f, 1.0f, 0.0f }; // fade to invisible
};

class ParticleSystem
{
public:
	// Emit `count` particles outward from `position`, with a base
	// random direction on the XZ plane (small upward Y component for
	// readability) scaled by speed. Lifetime + color lerp endpoints
	// control the visual.
	void Burst(const DirectX::XMFLOAT3& position,
		int count,
		float speed,
		float lifetime,
		const DirectX::XMFLOAT4& colorStart,
		const DirectX::XMFLOAT4& colorEnd,
		float size = 0.25f) noexcept;

	// Advance every particle's age + position by dt, remove expired
	// entries. Called once per frame by FLApp.
	void Tick(float dt) noexcept;

	[[nodiscard]] const std::vector<Particle>& Particles() const noexcept { return particles_; }
	[[nodiscard]] std::size_t Count() const noexcept { return particles_.size(); }

private:
	std::vector<Particle> particles_;
};
