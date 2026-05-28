#pragma once

#include <string>

// Marks a SceneObject as something the player can ride (horse, donkey,
// boar, etc.). Sibling to NpcComponent — same "approachable interactable"
// shape, but the verb is "Mount" instead of "Talk to" and the action wires
// to game-side mount mechanics rather than the dialog system.
//
// `displayName` flows to the HUD prompt ("[E] Mount Horse"). Empty value
// falls back to the SceneObject's own name.
//
// `speedMultiplier` is what the game-side mount logic uses to scale the
// rider's move speed while mounted. Default 1.6 matches a comfortable
// "noticeably faster than walking, not so fast that the player overshoots
// targets" feel.
//
// `occupied` is runtime-only state — true while a rider is on board. The
// engine doesn't read it; game-side PlayerController / MountSystem flips
// it to gate concurrent riders and to suppress re-mount prompts.
//
// Why this lives in the engine alongside NpcComponent: same rationale —
// the "approachable object" shape is generic enough to belong here, while
// what happens when the player triggers it (a quest dialog, a shop, a
// mount transition) is a game-policy decision the game layer owns.
struct MountComponent
{
	std::string displayName;
	float       speedMultiplier = 1.6f;
	bool        occupied        = false;
};
