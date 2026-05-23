#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
	float Rand01() noexcept
	{
		return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	}
}

void ParticleSystem::Burst(const DirectX::XMFLOAT3& position,
	int count,
	float speed,
	float lifetime,
	const DirectX::XMFLOAT4& colorStart,
	const DirectX::XMFLOAT4& colorEnd,
	float size) noexcept
{
	if (count <= 0) return;
	particles_.reserve(particles_.size() + static_cast<std::size_t>(count));
	for (int i = 0; i < count; ++i)
	{
		Particle p;
		p.position = position;
		// Random direction on the XZ disc, small upward Y kick so the
		// particle pops off the floor slightly before falling under
		// the velocity's natural decay.
		const float angle = Rand01() * 6.2831853f;
		const float r     = 0.5f + Rand01() * 0.5f;
		p.velocity = {
			std::cos(angle) * speed * r,
			0.8f + Rand01() * 1.4f,    // gentle upward
			std::sin(angle) * speed * r,
		};
		p.age    = 0.0f;
		p.maxAge = lifetime * (0.8f + Rand01() * 0.4f); // ±20% variance
		p.size   = size * (0.7f + Rand01() * 0.6f);
		p.colorStart = colorStart;
		p.colorEnd   = colorEnd;
		particles_.push_back(p);
	}
}

void ParticleSystem::Tick(float dt) noexcept
{
	if (dt <= 0.0f) return;
	// Move + age in one pass; we don't reorder because particles
	// don't interact and the render layer iterates the same vector.
	constexpr float kGravity = -3.0f; // m/s² downward — gentle, particles arc back to ground
	constexpr float kDrag    = 1.2f;  // velocity decay per second (XZ feels sluggish without this)

	for (Particle& p : particles_)
	{
		p.age += dt;
		// Apply drag to XZ — keeps particles from flying across the
		// screen at full launch speed.
		const float dragFactor = std::max(0.0f, 1.0f - kDrag * dt);
		p.velocity.x *= dragFactor;
		p.velocity.z *= dragFactor;
		// Gravity on Y, but clamp so a particle resting at y≈0 doesn't
		// sink into the ground; ground plane is at y=0.
		p.velocity.y += kGravity * dt;
		p.position.x += p.velocity.x * dt;
		p.position.y += p.velocity.y * dt;
		p.position.z += p.velocity.z * dt;
		if (p.position.y < 0.05f)
		{
			p.position.y = 0.05f;
			if (p.velocity.y < 0.0f) p.velocity.y = 0.0f;
		}
	}

	particles_.erase(
		std::remove_if(particles_.begin(), particles_.end(),
			[](const Particle& p) noexcept { return p.age >= p.maxAge; }),
		particles_.end());
}
