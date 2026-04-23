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
}
