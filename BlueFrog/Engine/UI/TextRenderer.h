#pragma once

#include "../../Core/Graphics.h"
#include "../Camera/TopDownCamera.h"
#include "../Scene/Scene.h"
#include "DamagePopup.h"
#include "HudState.h"
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

// One inventory slot: an icon (path, may be empty), a name, and a count.
struct UiSlot
{
    std::wstring icon;
    std::wstring label;
    int          count = 0;
};

// One crafting recipe row for the crafting panel.
struct UiCraftIngredient { std::wstring icon; int have = 0; int need = 0; };
struct UiCraftRecipe
{
    std::wstring outIcon;
    std::wstring name;
    int          outCount = 1;
    std::vector<UiCraftIngredient> inputs;
    bool         affordable = false;
};

// Draws the in-viewport HUD text overlay (objective text in β-1; HP numerics
// added in β-2). Lives on top of UIRenderer's HLSL quads — D2D BeginDraw /
// EndDraw is bracketed by Graphics::BeginTextDraw / EndTextDraw, which the
// caller invokes after the normal 3D + UI passes and before Present.
class TextRenderer final
{
public:
    explicit TextRenderer(Graphics& gfx);
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    // Draws the overlay. Safe to call when the D2D target is null (recreate
    // pending); in that case nothing happens and the caller's EndTextDraw
    // returns S_OK. `inspectorOpen` shrinks the objective text's layout
    // rect so it fits to the left of the inspector panel when present.
    // `damageFlashAlpha` (0..1) controls a transient fullscreen red
    // overlay that fades out after the player takes damage; 0 = no flash.
    void Render(const HudState& hud, int viewportW, int viewportH, bool inspectorOpen = false, float damageFlashAlpha = 0.0f) noexcept;

    // Bottom-center "[E] Talk to <name>" prompt. Drawn behind dialog UI
    // (so dialog box covers it when active). No-op when hud doesn't
    // currently carry a prompt.
    void RenderInteractPrompt(const HudState& hud, int viewportW, int viewportH) noexcept;

    // Dialog box: translucent panel docked along the bottom edge with
    // the NPC's name as a header and one line of dialogText below.
    // Painted on top of the interact prompt (so the prompt visually
    // disappears once dialog opens). When `npcName` is empty or
    // `alpha` <= 0 the call is a no-op — caller controls when dialog
    // is "active". `alpha` (0..1) scales the opacity of every brush
    // used for the dialog so the caller can fade-in / fade-out.
    void RenderDialog(const std::wstring& npcName, const std::wstring& text, int viewportW, int viewportH, float alpha) noexcept;

    // Inventory panel — centered modal listing the player's owned
    // items. Each entry in `lines` is one row of pre-formatted text
    // (e.g. "Healing Potion  x1"); the game side does the lookup-
    // and-format work so this stays engine-agnostic about what an
    // item actually is. Empty `lines` renders the "(empty)"
    // placeholder. `alpha` mirrors RenderDialog's fade param.
    void RenderInventory(const std::vector<UiSlot>& slots, int viewportW, int viewportH, float alpha) noexcept;

    // Crafting panel — recipe rows with output icon + ingredient icons and a
    // [1-9] Craft footer. The game side fills UiCraftRecipe (engine stays
    // agnostic about recipes/items).
    void RenderCrafting(const std::vector<UiCraftRecipe>& recipes, int viewportW, int viewportH, float alpha) noexcept;

    // Placement-tool overlay (world editor): a top-left panel showing the
    // current prefab + index, rotation, placed count, and key hints. Drawn
    // only while placement mode is active.
    void RenderPlacementHud(const std::wstring& prefabLabel, int prefabIndex, int prefabCount,
        float yawDegrees, bool gridSnap, int placedCount, int viewportW, int viewportH) noexcept;

    // Live look-tuner readout: a small bottom-left panel showing the current
    // post-process grade + sun values while the art look is being dialed in.
    void RenderGradeReadout(float exposure, float saturation, float contrast,
        float timeOfDay, int viewportW, int viewportH) noexcept;

    // Floating damage-number overlay. Projects each popup's spawn worldPos
    // through the supplied camera, then floats the resulting screen point
    // upward over the popup's lifetime and fades it out at the tail end.
    // Off-screen / behind-camera popups are silently skipped. Safe to call
    // with an empty vector; the D2D target may be null while a recreate
    // is pending, in which case the call is a no-op.
    void RenderDamagePopups(
        const std::vector<DamagePopup>& popups,
        const TopDownCamera& camera,
        int viewportW,
        int viewportH) noexcept;

    // Right-side scene inspector panel. Renders a translucent dark column,
    // a per-object summary list with component flags, and a detail dump for
    // the selected object. App owns the toggle/selection state; this just
    // draws what it is asked to draw. fieldIndex selects which transform/
    // combat field is the live-edit cursor (highlighted in the panel).
    void RenderInspector(const Scene& scene, int selectedIndex, int fieldIndex, int viewportW, int viewportH) noexcept;
private:
    // Lazily loads (and caches) a 64x64 icon PNG as a D2D bitmap. Returns
    // nullptr on a missing/failed load. The cache is keyed by path and tied to
    // the current D2D target -- when the target is recreated (resize) the
    // cache is dropped so we never draw a bitmap owned by a dead target.
    ID2D1Bitmap* GetIcon(const std::wstring& path) noexcept;
    ID2D1RenderTarget* iconCacheTarget_ = nullptr;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID2D1Bitmap>> iconCache_;

    Graphics& gfx;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    objectiveFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    numericFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    defeatedFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    popupFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    promptFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    dialogNameFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    dialogBodyFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    inventoryHeaderFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    inventoryRowFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    inspectorFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>    inspectorTitleFormat;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> whiteBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> redBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> popupBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> dimBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> highlightBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> panelBrush;
};
