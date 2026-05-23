#pragma once

#include <string>

// Marks a SceneObject as an NPC the player can interact with. The
// presence of this component (via std::optional on SceneObject) is what
// InteractionSystem keys on when scanning for the closest interactable.
//
// `displayName` is what the dialog box's title bar shows ("Villager",
// "Merchant", "Old Hermit", …). When empty, InteractionSystem falls back
// to the SceneObject's own name.
//
// `dialogText` is the v1 single-line dialog payload. A future Phase-I
// expansion will replace this with a dialog graph (ID + branching lines)
// loaded from a separate data file, but for the first NPC-meets-player
// experience a plain ASCII string per NPC is sufficient. The string is
// authored in the scene/prefab JSON and rendered as-is.
//
// Why this lives in the engine (not the game): an NPC's "approachable
// object with a name and a line of text" shape is generic enough that
// any BlueFrog-powered game will want it. Game-specific dialog logic
// (quest hooks, faction reactions, branching) layers on top — likely as
// additional components on the same SceneObject — without touching this.
struct NpcComponent
{
	std::string displayName;
	std::string dialogText;

	// Optional reference to a quest in the game-side QuestRegistry.
	// Empty = NPC has no quest, dialogText is used verbatim. Non-empty
	// = the game layer routes dialog through QuestSystem (the NPC
	// becomes the giver / turn-in target for the referenced quest)
	// and dialogText becomes the fallback when the quest doesn't
	// supply a status-specific line for the current state.
	//
	// Engine doesn't know what a quest is — this field is just a
	// string the game-side layer consumes. Loose coupling matches
	// how SceneLoader already ferries "objective" blocks as raw JSON.
	std::string questId;
};
