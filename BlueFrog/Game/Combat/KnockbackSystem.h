#pragma once

#include "../../Engine/Scene/Scene.h"

// Per-tick processor for pending knockback impulses sitting on every
// CombatComponent in the scene. CombatSystem::ApplyDamage seeds the
// component's knockbackVelocityXZ + knockbackTimeRemaining on a successful
// hit; this system is what actually MoveAndSlides the affected object,
// counts the timer down, and clears the velocity when the stun ends.
//
// Why a separate system instead of folding it into PlayerController /
// SimpleEnemyController: knockback applies uniformly to anything with a
// CombatComponent, including the boss, future neutrals, and the player.
// Centralizing the motion here means new combatant types get knockback
// "for free" without per-controller plumbing, and the per-tick ordering
// is a single, obvious line in GameplaySimulation::Update.
//
// Ordering contract (see GameplaySimulation::Update): runs AFTER the
// player + enemy systems have committed their own movement intent for the
// tick, so the knockback slide is layered on top of whatever the actor
// "wanted" to do — though behaviors are expected to short-circuit their
// own motion while knockbackTimeRemaining > 0 (i.e. stunned actors don't
// chase or attack). Runs BEFORE the trigger system so a knocked-back
// player can still trip a boundary trigger this same tick.
class KnockbackSystem final
{
public:
	static void Tick(Scene& scene, float dt) noexcept;
};
