#include "TextRenderer.h"

#include "TextLayout.h"
#include "UILayout.h"

#include <string>

namespace
{
    std::wstring FormatMeter(const HudMeter& meter)
    {
        return std::to_wstring(static_cast<int>(meter.current))
            + L"/"
            + std::to_wstring(static_cast<int>(meter.max));
    }
}

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

    // HP numerics: placed just to the right of each bar, vertically centered
    // on the bar row. Bar rects are expressed in NDC by UILayout — we mirror
    // that here so bar + text stay in lockstep if either constant changes.
    if (numericFormat)
    {
        const float barHalfHeightDip = UiLayout::PlayerHealthHeight * 0.5f * static_cast<float>(viewportH);

        const float playerRightNdc = UiLayout::PlayerHealthCenterX + UiLayout::PlayerHealthWidth * 0.5f;
        const float playerRightPx = TextLayout::NdcToPixelX(playerRightNdc, viewportW);
        const float playerCenterYpx = TextLayout::NdcToPixelY(UiLayout::PlayerHealthCenterY, viewportH);

        const std::wstring playerText = FormatMeter(hud.playerHealth);
        const D2D1_RECT_F playerRect = D2D1::RectF(
            playerRightPx + TextLayout::HealthNumericGapDip,
            playerCenterYpx - barHalfHeightDip,
            playerRightPx + TextLayout::HealthNumericGapDip + TextLayout::HealthNumericWidthDip,
            playerCenterYpx + barHalfHeightDip);
        target->DrawText(
            playerText.c_str(),
            static_cast<UINT32>(playerText.size()),
            numericFormat.Get(),
            playerRect,
            whiteBrush.Get(),
            D2D1_DRAW_TEXT_OPTIONS_NONE,
            DWRITE_MEASURING_MODE_NATURAL);

        if (hud.hasTarget)
        {
            const float targetRightNdc = UiLayout::TargetHealthCenterX + UiLayout::TargetHealthWidth * 0.5f;
            const float targetRightPx = TextLayout::NdcToPixelX(targetRightNdc, viewportW);
            const float targetCenterYpx = TextLayout::NdcToPixelY(UiLayout::TargetHealthCenterY, viewportH);

            const std::wstring targetText = FormatMeter(hud.targetHealth);
            const D2D1_RECT_F targetRect = D2D1::RectF(
                targetRightPx + TextLayout::HealthNumericGapDip,
                targetCenterYpx - barHalfHeightDip,
                targetRightPx + TextLayout::HealthNumericGapDip + TextLayout::HealthNumericWidthDip,
                targetCenterYpx + barHalfHeightDip);
            target->DrawText(
                targetText.c_str(),
                static_cast<UINT32>(targetText.size()),
                numericFormat.Get(),
                targetRect,
                whiteBrush.Get(),
                D2D1_DRAW_TEXT_OPTIONS_NONE,
                DWRITE_MEASURING_MODE_NATURAL);
        }
    }
}
