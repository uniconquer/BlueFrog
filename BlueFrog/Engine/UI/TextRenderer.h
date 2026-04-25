#pragma once

#include "../../Core/Graphics.h"
#include "HudState.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

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
    // returns S_OK.
    void Render(const HudState& hud, int viewportW, int viewportH) noexcept;
private:
    Graphics& gfx;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> objectiveFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> numericFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> defeatedFormat;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> whiteBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> redBrush;
};
