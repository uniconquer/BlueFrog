#pragma once

#include <string>

// Per-instance animation playback state for a SceneObject whose render path
// resolves to a skinned mesh. Stage 4a (foundation):
//
//   - `clipName`: which clip in the mesh's `animations[]` to play. Empty
//     string means "first clip" so RiggedSimple-shaped assets that don't
//     name their clip work without scene-side configuration.
//   - `clipTime`: seconds into the current clip. AnimationSystem ticks
//     this each frame by `dt * playSpeed`; the renderer wraps it via
//     fmod(clipTime, clip.duration) when sampling.
//   - `playSpeed`: 1.0 plays at authored rate. Negative reverses; zero
//     freezes (useful as a debug pause).
//   - `looping`: when false, clipTime stops advancing once it reaches
//     `clip.duration` (the renderer pins to the last keyframe).
//
// Stage 4b (state machine, follow-up commit) layers a controller on top
// that mutates `clipName` based on gameplay events (Idle / Walk / Attack
// transitions) and adds a crossfade timer.
struct AnimationStateComponent
{
	std::string clipName;
	float       clipTime  = 0.0f;
	float       playSpeed = 1.0f;
	bool        looping   = true;

	// --- Crossfade state (managed entirely by AnimationSystem) ---
	// Gameplay systems only ever write `clipName`. AnimationSystem watches
	// for it to differ from `activeClip` (what's actually playing); on a
	// change it snapshots the outgoing clip + time into blendFrom*, resets
	// clipTime to 0 for the new clip, and starts blendRemaining counting
	// down from blendDuration. While blendRemaining > 0 the renderer
	// samples BOTH clips and lerps/slerps the joint poses, so transitions
	// (Slash->Idle, Idle->Walk, Walk->Ride, ...) melt instead of popping.
	std::string activeClip;
	std::string blendFromClip;
	float       blendFromTime  = 0.0f;
	float       blendRemaining = 0.0f;
	float       blendDuration  = 0.15f;
};
