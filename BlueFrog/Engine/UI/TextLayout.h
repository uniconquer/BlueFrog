#pragma once

// Layout constants for the in-viewport text overlay (DirectWrite via D2D).
//
// Font sizes are authored in points and converted to DIPs for DWrite, which
// expects device-independent-pixel sizes. The Point→DIP conversion is the
// standard 1 pt = 96/72 DIP.
//
// Keep this header data-only: no includes beyond what constexpr requires.

namespace TextLayout
{
    inline constexpr float ObjectivePointSize    = 20.0f;
    inline constexpr float NumericPointSize      = 14.0f;
    inline constexpr float ObjectiveTopInsetDip  = 16.0f;

    inline constexpr wchar_t kFontFamily[] = L"Segoe UI";
    inline constexpr wchar_t kFontLocale[] = L"en-us";

    inline constexpr float PointsToDips(float pt) noexcept
    {
        return pt * 96.0f / 72.0f;
    }

    // Gap (in DIP) between the right edge of an HP bar and the start of its
    // numeric readout.
    inline constexpr float HealthNumericGapDip = 8.0f;

    // Max layout-rect width for HP numerics. 80 DIP comfortably fits strings
    // like "999/999" at NumericPointSize without clipping.
    inline constexpr float HealthNumericWidthDip = 80.0f;

    // NDC is x=[-1,+1] left→right, y=[+1,-1] top→bottom (flipped). Convert to
    // D2D pixel coordinates (origin top-left).
    inline float NdcToPixelX(float ndc, int viewportW) noexcept
    {
        return (ndc * 0.5f + 0.5f) * static_cast<float>(viewportW);
    }
    inline float NdcToPixelY(float ndc, int viewportH) noexcept
    {
        return (1.0f - (ndc * 0.5f + 0.5f)) * static_cast<float>(viewportH);
    }
}
