#pragma once
#include "../../Engine/Camera/TopDownCamera.h"
#include "../../Engine/Scene/Scene.h"
#include "../../Engine/UI/DamagePopup.h"
#include "PlayerMovementSystem.h"
#include "../Simulation/GameplayInput.h"
#include <string>
#include <vector>

class EventBus;
class AudioEngine;
class SkillSystem;

class PlayerController
{
public:
	bool Update(const GameplayInput& input, Scene& scene, TopDownCamera& camera, float dt, EventBus& bus, AudioEngine* audio, std::vector<DamagePopup>* popups, SkillSystem* skills) noexcept;
	float GetAttackCooldownProgress01() const noexcept;
	float GetHeavyAttackCooldownProgress01() const noexcept;

	// Mount lifecycle (Phase Mount).
	// Mount(name) attaches the player to the named SceneObject (its
	// MountComponent.occupied flag is also flipped by the caller). While
	// mounted, the player rides on top of the mount: movement input drives
	// the mount's transform, the player follows along, and attacks/dash
	// are disabled. Empty mountedOnName == not mounted.
	void SetMount(const std::string& mountObjectName) noexcept { mountedOnName = mountObjectName; mountSpeed = 0.0f; }
	void ClearMount() noexcept { mountedOnName.clear(); mountSpeed = 0.0f; }
	[[nodiscard]] bool IsMounted() const noexcept { return !mountedOnName.empty(); }
	[[nodiscard]] const std::string& MountedOn() const noexcept { return mountedOnName; }
private:
	SceneObject* FindPlayer(Scene& scene) noexcept;
	void UpdateTint(SceneObject& player) const noexcept;
private:
	static constexpr float moveSpeed = 6.5f;
	// Y position the player snaps to each tick. 0 = feet on the ground
	// plane (Phase F Stage 4c switched the player from a center-pivoted
	// cube to a feet-pivoted skinned character mesh).
	static constexpr float playerHeight = 0.0f;

	// Cached pointer to the SkillSystem so GetAttackCooldownProgress01
	// (called by HudPresenter) can answer without needing its own
	// ctx-threading. Updated every Update tick.
	SkillSystem* skillsCached = nullptr;

	// Dash: short burst at higher speed in the current movement direction
	// (or facing if no movement input). Fixed window of dashDuration with
	// a recovery window of dashCooldown after — held-key sampling means
	// the player can chain dashes back-to-back at the cooldown rate.
	static constexpr float dashSpeedMul   = 3.0f; // multiplied by moveSpeed
	static constexpr float dashDuration   = 0.18f;
	static constexpr float dashCooldown   = 0.55f;
	float dashTimeRemaining     = 0.0f;
	float dashCooldownRemaining = 0.0f;
	float dashDirX              = 0.0f;
	float dashDirZ              = 0.0f;

	// Mount state — name of the SceneObject the player is currently
	// riding. Empty when not mounted. PlayerController.Update branches on
	// this: mounted updates drive the mount's transform and sync the
	// player on top, dismounted updates are the legacy walk path.
	std::string mountedOnName;
	// Vertical offset (meters) applied to player.position.y while mounted
	// so the rider visually sits ON the mount rather than inside it. The
	// Quaternius horse at importScale 0.5 has the saddle ridge ~1.0m off
	// the ground; the Ride clip's bent-knee pose lifts the feet ~0.2m
	// above the player root, so 1.2m parks the feet just above the
	// saddle without floating.
	static constexpr float mountRiderYOffset = 1.2f;

	// Mount "vehicle" dynamics — gives the horse a GTA-style weighty feel
	// instead of instant start/stop. mountSpeed is the current scalar
	// speed (m/s) along the mount's forward axis; it ramps up under input
	// and coasts down by friction when input releases, so the horse
	// glides to a halt rather than freezing mid-stride. Reset to 0 on
	// mount/dismount (see SetMount/ClearMount).
	float mountSpeed = 0.0f;
	static constexpr float mountAccel = 9.0f;  // m/s^2 ramp-up under input
	static constexpr float mountDecel = 7.0f;  // m/s^2 coast-down (friction)

public:
	// Vertical layer (Jump V1) — read by the animation controller, camera
	// and landing-FX wiring.
	[[nodiscard]] bool  IsAirborne() const noexcept { return !grounded; }
	[[nodiscard]] float VerticalVelocity() const noexcept { return verticalVelocity; }
	[[nodiscard]] bool  IsLandingStunned() const noexcept { return landingStunRemaining > 0.0f; }
	// One-shot: true exactly once after a >= kHardLandHeight fall lands
	// (the caller spawns dust/SFX off it).
	[[nodiscard]] bool  ConsumeHardLanded() noexcept { const bool v = hardLandedThisTick; hardLandedThisTick = false; return v; }
private:
	// Gravity integration + jump with coyote time and an input buffer.
	// The player's y is no longer snapped to 0 — it rides FloorHeightAt
	// (stairs, crates, roofs) and ballistic arcs between. Tuning per the
	// V1 design: snappy arcade gravity, ~1.3m apex.
	void IntegrateVertical(SceneObject& player, Scene& scene, float dt, bool allowJump) noexcept;
	static constexpr float kGravity         = -22.0f;
	static constexpr float kJumpVelocity    = 7.5f;   // apex ~= v^2/2g ~= 1.3m
	static constexpr float kAirControlMul   = 0.8f;
	static constexpr float kCoyoteTime      = 0.10f;
	static constexpr float kJumpBufferTime  = 0.12f;
	static constexpr float kHardLandHeight  = 3.5f;   // falls >= this stun on landing
	static constexpr float kHardLandStun    = 0.4f;
	float verticalVelocity      = 0.0f;
	bool  grounded              = true;
	float coyoteRemaining       = 0.0f;
	float jumpBufferRemaining   = 0.0f;
	bool  prevJumpHeld          = false;
	float airApexY              = 0.0f;
	float landingStunRemaining  = 0.0f;
	bool  hardLandedThisTick    = false;
};
