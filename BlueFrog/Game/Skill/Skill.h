#pragma once

#include <string>
#include <vector>

// One frame-keyed beat in a skill's timeline. `time` is in seconds
// from skill start; SkillSystem fires the event the first tick that
// elapsed crosses it. v1 event types:
//   "damage"   — calls CombatSystem against the closest enemy within
//                `range` for `amount` HP.
//   "particle" — bursts a small splash at the caster's position.
struct SkillEvent
{
	std::string type;
	float       time   = 0.0f;
	int         amount = 0;    // for damage
	float       range  = 0.0f; // for damage
};

// Static definition of a skill. Loaded once at boot into SkillRegistry;
// SkillSystem holds per-caster runtime state separately.
struct Skill
{
	std::string id;
	std::string name;
	std::string animationClip; // clip name on the caster's mesh
	float       duration  = 0.6f; // total skill length (seconds)
	float       cooldown  = 0.45f;
	std::vector<SkillEvent> events;
};
