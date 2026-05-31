#pragma once

#include "AppBase.h"

#include "Renderer.h"
#include "../Engine/Camera/TopDownCamera.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/UI/DamagePopup.h"
#include "../Engine/UI/HudState.h"
#include "../Engine/UI/UIRenderer.h"
#include "../Engine/UI/TextRenderer.h"
#include "../Engine/Render/DebugRenderer.h"
#include "../Engine/Render/ParticleRenderer.h"
#include "../Engine/Render/ParticleSystem.h"
#include "../Engine/Render/PostProcessPass.h"
#include "../Engine/Render/WorldGridRenderer.h"
#include "../Engine/Audio/AudioEngine.h"
#include "../Game/Simulation/GameplaySimulation.h"
#include "../Game/Inventory/Inventory.h"
#include "../Game/Inventory/ItemRegistry.h"
#include "../Game/Profile/PlayerProfile.h"
#include "../Game/Quest/QuestRegistry.h"
#include "../Game/Quest/QuestSystem.h"
#include "../Game/Skill/SkillRegistry.h"
#include "../Game/Skill/SkillSystem.h"
#include <string>
#include <vector>

// FL game-side application. Subclasses BlueFrogEngine's AppBase to plug
// the game's renderers, scene, simulation, audio, and HUD into the
// engine's per-frame hooks. AppBase owns the OS window + main loop; this
// class owns everything specific to FL.
//
// Why a subclass instead of composition: matches Unity's MonoBehaviour
// and Unreal's AActor::Tick pattern — game code lives in virtual
// overrides of well-known lifecycle hooks (OnStartup / OnUpdate /
// OnRender / OnShutdown). The next FL-or-other game can swap this whole
// class out and the engine doesn't change.
class FLApp final : public AppBase
{
public:
	explicit FLApp(std::string scenePath = {});

protected:
	void OnStartup() override;
	void OnUpdate(float dt) override;
	void OnRender() override;

private:
	void UpdateModel(const GameplayInput& input, float dt) noexcept;
	GameplayInput CollectGameplayInput(float dt) noexcept;
	void PollDebugToggles() noexcept;
	void ApplyQuestReward(const struct QuestReward& reward) noexcept;
	// Consume one of the (currently hardcoded) "healing_potion" item
	// from inventory and apply its ItemEffect to the player. No-op
	// when the inventory is empty. Phase I-3D will generalize to
	// hotbar slot → item-id mapping.
	void UseConsumable() noexcept;
	// Restore the dialog NPC's authored facing (saved when the dialog
	// opened) and clear the tracking state. No-op when no NPC was faced.
	void RestoreDialogFacing(Scene& scene) noexcept;
	// Placement tool: consume the cycle/rotate/undo edges and, on LMB, drop
	// the selected prefab at the mouse's ground point. Returns the current
	// prefab's display label for the HUD/title.
	void UpdatePlacement(const GameplayInput& input) noexcept;
	[[nodiscard]] const char* PlacementPrefabPath() const noexcept;

	Renderer renderer;
	UIRenderer uiRenderer;
	TextRenderer textRenderer;
	DebugRenderer debugRenderer;
	ParticleRenderer particleRenderer;
	WorldGridRenderer worldGridRenderer;
	PostProcessPass postProcess;
	ParticleSystem particleSystem;
	TopDownCamera camera;
	Scene scene;
	HudState hudState;
	GameplaySimulation gameplaySimulation;
	AudioEngine audio;
	// Quest layer (Phase I-2A). Registry holds the static quest
	// definitions loaded from Assets/Quests/*.quest.json at boot;
	// QuestSystem holds the per-quest runtime state (accepted /
	// progress / turned-in) and survives scene reloads, since a
	// quest can span multiple scenes (e.g. arena_trial conditions
	// progressed while talking to a villager back in village).
	QuestRegistry questRegistry;
	QuestSystem   questSystem;

	// Inventory layer (Phase I-3A). ItemRegistry holds static item
	// definitions (loaded once from Assets/Items/*.item.json);
	// Inventory holds the per-player runtime ownership. Both
	// survive scene reloads since FLApp itself does. Phase I-3D /
	// future profile-save commit will persist Inventory across
	// launches.
	ItemRegistry itemRegistry;
	Inventory    inventory;

	// Skill layer. Registry holds static definitions loaded from
	// Assets/Skills/*.skill.json; SkillSystem owns per-actor
	// execution state (mid-skill elapsed, fired event flags,
	// cooldowns) across scene reloads.
	SkillRegistry skillRegistry;
	SkillSystem   skillSystem;
	std::string currentScenePath;
	bool   debugGizmosEnabled  = false;
	bool   worldGridEnabled    = false;
	bool   reloadRequested     = false;
	// In-game placement tool (world editor). F4 toggles; in placement mode
	// LMB drops the selected prefab at the mouse's ground point, [ ] cycle
	// the prefab, T rotates, Backspace undoes the last drop, F12 saves.
	bool        placementMode    = false;
	int         placementIndex   = 0;
	float       placementYaw     = 0.0f;
	int         placementCounter = 0;
	bool        placeCycleNext   = false;
	bool        placeCyclePrev   = false;
	bool        placeRotate      = false;
	bool        placeUndo        = false;
	std::vector<std::string> placedNames;
	bool   inspectorEnabled    = false;
	int    inspectorSelected   = 0;
	int    inspectorFieldIndex = 0;
	// Damage feedback: when player HP drops between ticks we light up a
	// transient fullscreen red overlay. lastPlayerHealth tracks the value
	// from the previous tick (-1 = uninitialized); damageFlashAlpha
	// linearly fades to 0 over kDamageFlashDuration after each hit.
	int    lastPlayerHealth   = -1;
	float  damageFlashAlpha   = 0.0f;
	// Persistent profile state (Phase H Stage 1). currentPlayTimeSec
	// accumulates monotonically across the session; F8 snapshots it
	// alongside scene + HP into the save file. The Save folder lives
	// outside the build outputs so a clean rebuild doesn't wipe progress.
	float  currentPlayTimeSec = 0.0f;
	// Live floating damage-number popups. Combat code appends new entries
	// via the SystemContext sink; FLApp ticks ages each frame and erases
	// expired ones before TextRenderer projects the remainder. Owned here
	// (rather than on GameplaySimulation) so a scene transition does not
	// drop in-flight popups — though in practice ReloadScene also resets
	// HP and downstream feel, so a clear-on-reload is fine too.
	std::vector<DamagePopup> activePopups;
	// Hit-impact screen shake. shakeMagnitude is in world units (XZ plane
	// translational offset on the camera). OnUpdate kicks it on detected
	// hits — player took damage = heavy kick, popup count grew while HP
	// held steady = the player landed one (lighter kick). shakeTimer
	// advances each frame; the actual per-frame offset is sin(timer*freq)
	// * magnitude * sign in a randomly-rotated direction set at kick time.
	// lastPopupCount is the comparison baseline for the popup delta.
	float  shakeMagnitude   = 0.0f;
	float  shakeTimer       = 0.0f;
	float  shakeDirX        = 1.0f;
	float  shakeDirZ        = 0.0f;
	size_t lastPopupCount   = 0;

	// Dialog mode (Phase I-1C). When `dialogActive` is true, TextRenderer
	// paints a bottom dialog box with the captured NPC's name + line.
	// E key edge-toggles the mode. Dialog also freezes the simulation
	// (Polish 3): GameplaySimulation::Update sees dt=0 so movement,
	// combat, animation, and triggers all hold their state until the
	// dialog closes. dialogFade tracks the 0→1 fade-in animation.
	bool          dialogActive = false;
	std::wstring  dialogNpcName;
	std::wstring  dialogText;
	float         dialogFade   = 0.0f;
	// While a dialog is open the engaged NPC turns to face the player, then
	// restores its authored facing on close. Track the NPC's scene name and
	// the yaw it had before we rotated it so the restore is exact.
	std::string   dialogFacingNpc;
	float         dialogFacingSavedYaw = 0.0f;

	// Inventory UI mode (Phase I-3B). I key edge-toggles the panel.
	// Mutually exclusive with dialog mode — opening one closes the
	// other so the player isn't stuck in two modals at once. Both
	// modals pause the simulation.
	bool          inventoryActive = false;
	float         inventoryFade   = 0.0f;
	// I-key edge detection. Parallel to interactPressedThisFrame
	// for the E key — PollDebugToggles latches, CollectGameplayInput
	// reads + clears.
	bool          inventoryKeyPressedThisFrame = false;
	// '1' hotkey edge for the first consumable slot.
	bool          consumeHotkeyPressedThisFrame = false;
	// E-key edge detection. PollDebugToggles consumes the keyboard event
	// queue and sets this flag for the current frame; CollectGameplayInput
	// reads it into the GameplayInput, then we clear it.
	bool          interactPressedThisFrame = false;
	// F-key edge for the second skill slot (heavy_slash).
	bool          heavyAttackPressedThisFrame = false;
};
