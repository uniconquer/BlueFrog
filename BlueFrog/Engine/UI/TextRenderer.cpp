#include "TextRenderer.h"

#include "TextLayout.h"

TextRenderer::TextRenderer(Graphics& gfxIn)
    :
    gfx(gfxIn)
{
    IDWriteFactory* const dwrite = gfx.GetDWriteFactory();
    ID2D1RenderTarget* const target = gfx.GetD2DTarget();

    if (dwrite == nullptr || target == nullptr)
    {
        // Graphics ctor ensures both are non-null on success; guard anyway so
        // a future hot-recreate path doesn't explode if we're instantiated
        // while the target is being rebuilt.
        return;
    }

    HRESULT hr = dwrite->CreateTextFormat(
        TextLayout::kFontFamily,
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        TextLayout::PointsToDips(TextLayout::ObjectivePointSize),
        TextLayout::kFontLocale,
        objectiveFormat.GetAddressOf());
    if (FAILED(hr))
    {
        throw BFGFX_EXCEPT(hr);
    }
    objectiveFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    objectiveFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    objectiveFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    hr = dwrite->CreateTextFormat(
        TextLayout::kFontFamily,
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        TextLayout::PointsToDips(TextLayout::NumericPointSize),
        TextLayout::kFontLocale,
        numericFormat.GetAddressOf());
    if (FAILED(hr))
    {
        throw BFGFX_EXCEPT(hr);
    }
    numericFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    numericFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    numericFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    if (FAILED(hr = target->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White, 1.0f),
        whiteBrush.GetAddressOf())))
    {
        throw BFGFX_EXCEPT(hr);
    }
}

void TextRenderer::Render(const HudState& hud, int viewportW, int viewportH) noexcept
{
    ID2D1RenderTarget* const target = gfx.GetD2DTarget();
    if (target == nullptr || !objectiveFormat || !whiteBrush)
    {
        return;
    }

    if (!hud.objectiveText.empty())
    {
        const D2D1_RECT_F layoutRect = D2D1::RectF(
            0.0f,
            TextLayout::ObjectiveTopInsetDip,
            static_cast<float>(viewportW),
            TextLayout::ObjectiveTopInsetDip + TextLayout::PointsToDips(TextLayout::ObjectivePointSize) * 1.5f);

        target->DrawText(
            hud.objectiveText.c_str(),
            static_cast<UINT32>(hud.objectiveText.size()),
            objectiveFormat.Get(),
            layoutRect,
            whiteBrush.Get(),
            D2D1_DRAW_TEXT_OPTIONS_NONE,
            DWRITE_MEASURING_MODE_NATURAL);
    }

    (void)viewportH; // β-2 will use this for HP numerics layout.
}
