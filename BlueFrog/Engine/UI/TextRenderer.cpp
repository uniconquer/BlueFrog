#include "TextRenderer.h"

#include "InspectorFields.h"
#include "TextLayout.h"
#include "UILayout.h"

#include "../Scene/SceneObject.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace
{
    std::wstring FormatMeter(const HudMeter& meter)
    {
        return std::to_wstring(static_cast<int>(meter.current))
            + L"/"
            + std::to_wstring(static_cast<int>(meter.max));
    }

    // Cheap ASCII narrow-to-wide. Scene/object names are ASCII by contract
    // (validator path-prefixed errors enforce this earlier), so we don't
    // need a real codecvt round trip here.
    std::wstring Widen(const std::string& s)
    {
        std::wstring out;
        out.reserve(s.size());
        for (char c : s)
        {
            out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
        }
        return out;
    }

    const wchar_t* FactionLabel(CombatFaction f)
    {
        switch (f)
        {
        case CombatFaction::Player:  return L"player";
        case CombatFaction::Enemy:   return L"enemy";
        case CombatFaction::Neutral:
        default:                     return L"neutral";
        }
    }

    const wchar_t* MeshLabel(RenderMeshType m)
    {
        switch (m)
        {
        case RenderMeshType::Plane: return L"plane";
        case RenderMeshType::Cube:
        default:                    return L"cube";
        }
    }

    // Build the "[TRCBGEA]" component-flag string for the per-object summary.
    std::wstring ComponentFlags(const SceneObject& obj)
    {
        wchar_t buf[8];
        buf[0] = L'T'; // Transform always present
        buf[1] = obj.renderComponent.has_value()         ? L'R' : L'-';
        buf[2] = obj.collisionComponent.has_value()      ? L'C' : L'-';
        buf[3] = obj.combatComponent.has_value()         ? L'B' : L'-';
        buf[4] = obj.triggerComponent.has_value()        ? L'G' : L'-';
        buf[5] = obj.enemyBehaviorComponent.has_value()  ? L'E' : L'-';
        buf[6] = obj.animationStateComponent.has_value() ? L'A' : L'-';
        buf[7] = L'\0';
        return std::wstring(buf);
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

    hr = dwrite->CreateTextFormat(
        TextLayout::kFontFamily,
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        TextLayout::PointsToDips(TextLayout::DefeatedPointSize),
        TextLayout::kFontLocale,
        defeatedFormat.GetAddressOf());
    if (FAILED(hr))
    {
        throw BFGFX_EXCEPT(hr);
    }
    defeatedFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    defeatedFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    defeatedFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    if (FAILED(hr = target->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White, 1.0f),
        whiteBrush.GetAddressOf())))
    {
        throw BFGFX_EXCEPT(hr);
    }
    if (FAILED(hr = target->CreateSolidColorBrush(
        D2D1::ColorF(0.95f, 0.18f, 0.18f, 1.0f),
        redBrush.GetAddressOf())))
    {
        throw BFGFX_EXCEPT(hr);
    }

    // Inspector resources. Consolas because the panel renders aligned
    // ASCII rows where proportional fonts would mis-align everything.
    hr = dwrite->CreateTextFormat(
        TextLayout::kInspectorFontFamily,
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        TextLayout::PointsToDips(TextLayout::InspectorPointSize),
        TextLayout::kFontLocale,
        inspectorFormat.GetAddressOf());
    if (FAILED(hr))
    {
        throw BFGFX_EXCEPT(hr);
    }
    inspectorFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    inspectorFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    inspectorFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    hr = dwrite->CreateTextFormat(
        TextLayout::kInspectorFontFamily,
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        TextLayout::PointsToDips(TextLayout::InspectorTitlePointSize),
        TextLayout::kFontLocale,
        inspectorTitleFormat.GetAddressOf());
    if (FAILED(hr))
    {
        throw BFGFX_EXCEPT(hr);
    }
    inspectorTitleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    inspectorTitleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    inspectorTitleFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    // Translucent dark slate for the inspector panel background. D2D handles
    // the pre-multiply for the PREMULTIPLIED render target.
    if (FAILED(hr = target->CreateSolidColorBrush(
        D2D1::ColorF(0.05f, 0.07f, 0.12f, 0.82f),
        panelBrush.GetAddressOf())))
    {
        throw BFGFX_EXCEPT(hr);
    }
    if (FAILED(hr = target->CreateSolidColorBrush(
        D2D1::ColorF(0.62f, 0.68f, 0.78f, 1.0f),
        dimBrush.GetAddressOf())))
    {
        throw BFGFX_EXCEPT(hr);
    }
    if (FAILED(hr = target->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 0.88f, 0.32f, 1.0f),
        highlightBrush.GetAddressOf())))
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

    if (hud.playerDefeated && defeatedFormat && redBrush)
    {
        // Big red center-screen "Defeated" text. The auto-reload timer in
        // GameplaySimulation gives this ~1.5s of screen time before the
        // scene resets — long enough to read, short enough to stay snappy.
        const wchar_t kDefeatedText[] = L"Defeated";
        const D2D1_RECT_F fullViewport = D2D1::RectF(
            0.0f,
            0.0f,
            static_cast<float>(viewportW),
            static_cast<float>(viewportH));
        constexpr UINT32 kDefeatedLen = (sizeof(kDefeatedText) / sizeof(wchar_t)) - 1u;
        target->DrawText(
            kDefeatedText,
            kDefeatedLen,
            defeatedFormat.Get(),
            fullViewport,
            redBrush.Get(),
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

void TextRenderer::RenderInspector(const Scene& scene, int selectedIndex, int fieldIndex, int viewportW, int viewportH) noexcept
{
    ID2D1RenderTarget* const target = gfx.GetD2DTarget();
    if (target == nullptr || !inspectorFormat || !inspectorTitleFormat || !panelBrush)
    {
        return;
    }

    const auto& objects = scene.GetObjects();
    const float panelW = TextLayout::InspectorPanelWidthDip;
    const float panelLeft  = static_cast<float>(viewportW) - panelW;
    const float panelTop   = 0.0f;
    const float panelRight = static_cast<float>(viewportW);
    const float panelBot   = static_cast<float>(viewportH);
    const float padX       = TextLayout::InspectorPanelMarginDip;
    const float lineH      = TextLayout::InspectorLineHeightDip;

    // 1. Translucent background.
    target->FillRectangle(D2D1::RectF(panelLeft, panelTop, panelRight, panelBot), panelBrush.Get());

    float y = panelTop + padX;

    // 2. Title + key-binding hint rows. Two short lines so each binding
    //    cluster fits without truncation.
    {
        const wchar_t titleBuf[] = L"INSPECTOR  F2 close";
        constexpr UINT32 titleLen = (sizeof(titleBuf) / sizeof(wchar_t)) - 1u;
        target->DrawText(titleBuf, titleLen, inspectorTitleFormat.Get(),
            D2D1::RectF(panelLeft + padX, y, panelRight - padX, y + lineH * 1.4f),
            whiteBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        y += lineH * 1.6f;

        const wchar_t hintBuf[] = L"Tab obj  PgUp/Dn field  L/R edit  +Shift x10";
        constexpr UINT32 hintLen = (sizeof(hintBuf) / sizeof(wchar_t)) - 1u;
        target->DrawText(hintBuf, hintLen, inspectorFormat.Get(),
            D2D1::RectF(panelLeft + padX, y, panelRight - padX, y + lineH),
            dimBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        y += lineH * 1.4f;
    }

    // 3. Object count.
    {
        wchar_t buf[64];
        const int n = std::swprintf(buf, 64, L"Objects: %zu", objects.size());
        if (n > 0)
        {
            target->DrawText(buf, static_cast<UINT32>(n), inspectorFormat.Get(),
                D2D1::RectF(panelLeft + padX, y, panelRight - padX, y + lineH),
                dimBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        }
        y += lineH * 1.4f;
    }

    // 4. Object list. Selected index is clamped here; App keeps it in
    //    [0, objects.size()) but a scene reload while the inspector is open
    //    can easily push it out of range.
    const int clampedSel = objects.empty()
        ? -1
        : std::max(0, std::min(selectedIndex, static_cast<int>(objects.size()) - 1));

    for (size_t i = 0; i < objects.size(); ++i)
    {
        const SceneObject& obj = objects[i];
        const std::wstring flags = ComponentFlags(obj);
        const std::wstring name = Widen(obj.name.empty() ? std::string("(unnamed)") : obj.name);

        wchar_t row[160];
        const wchar_t cursor = (static_cast<int>(i) == clampedSel) ? L'>' : L' ';
        const int n = std::swprintf(row, 160, L"%c %2zu [%s] %ls", cursor, i, flags.c_str(), name.c_str());
        if (n > 0)
        {
            ID2D1SolidColorBrush* brush = (static_cast<int>(i) == clampedSel) ? highlightBrush.Get() : whiteBrush.Get();
            target->DrawText(row, static_cast<UINT32>(n), inspectorFormat.Get(),
                D2D1::RectF(panelLeft + padX, y, panelRight - padX, y + lineH),
                brush, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        }
        y += lineH;

        // Page-overflow guard: if the list outgrows the panel before we even
        // reach the detail block, we just stop drawing further rows. This
        // keeps the renderer fail-safe; pagination is a future improvement.
        if (y > panelBot - lineH * 12.0f)
        {
            break;
        }
    }

    if (clampedSel < 0)
    {
        return;
    }

    // 5. Detail dump for the selected object.
    y += lineH * 0.6f;
    {
        wchar_t sep[80];
        const std::wstring name = Widen(objects[clampedSel].name);
        const int n = std::swprintf(sep, 80, L"--- %ls ---", name.c_str());
        if (n > 0)
        {
            target->DrawText(sep, static_cast<UINT32>(n), inspectorFormat.Get(),
                D2D1::RectF(panelLeft + padX, y, panelRight - padX, y + lineH),
                highlightBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        }
        y += lineH;
    }

    // Render one detail line. `editKind` is the field this line represents
    // for the live-edit cursor (or COUNT if the line is read-only). When
    // editKind matches the current fieldIndex, the line is highlighted and
    // prefixed with '*' so the user can see which value Left/Right will
    // mutate.
    const InspectorFields::Kind activeKind = static_cast<InspectorFields::Kind>(fieldIndex);
    auto DrawLine = [&](InspectorFields::Kind editKind, const wchar_t* fmt, auto... args)
    {
        if (y >= panelBot - lineH)
        {
            y += lineH;
            return;
        }
        wchar_t buf[160];
        const bool isEdit = (editKind != InspectorFields::Kind::COUNT) && (editKind == activeKind);
        const wchar_t prefix = isEdit ? L'*' : L' ';
        wchar_t lineBuf[176];
        // Inline the prefix into the format result to keep alignment stable.
        const int n = std::swprintf(buf, 160, fmt, args...);
        if (n <= 0)
        {
            y += lineH;
            return;
        }
        const int total = std::swprintf(lineBuf, 176, L"%c %ls", prefix, buf);
        if (total <= 0)
        {
            y += lineH;
            return;
        }
        ID2D1SolidColorBrush* brush = isEdit ? highlightBrush.Get() : whiteBrush.Get();
        target->DrawText(lineBuf, static_cast<UINT32>(total), inspectorFormat.Get(),
            D2D1::RectF(panelLeft + padX, y, panelRight - padX, y + lineH),
            brush, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        y += lineH;
    };

    const SceneObject& sel = objects[clampedSel];
    const auto& p = sel.transform.position;
    const auto& r = sel.transform.rotation;
    const auto& s = sel.transform.scale;
    // Position is split per-axis so each axis has its own edit row.
    DrawLine(InspectorFields::Kind::PosX, L"pos.x  %6.2f", p.x);
    DrawLine(InspectorFields::Kind::PosY, L"pos.y  %6.2f", p.y);
    DrawLine(InspectorFields::Kind::PosZ, L"pos.z  %6.2f", p.z);
    DrawLine(InspectorFields::Kind::RotY, L"rot.y  %6.2f  (rot.xz %5.2f %5.2f)", r.y, r.x, r.z);
    DrawLine(InspectorFields::Kind::COUNT, L"scale  %5.2f %5.2f %5.2f", s.x, s.y, s.z);
    DrawLine(InspectorFields::Kind::COUNT, L"enabled=%s", sel.enabled ? L"Y" : L"N");

    if (sel.renderComponent.has_value())
    {
        const auto& rc = sel.renderComponent.value();
        if (rc.material.has_value())
        {
            const auto& mat = rc.material.value();
            DrawLine(InspectorFields::Kind::COUNT, L"render mesh=%ls tint=(%.2f %.2f %.2f)",
                MeshLabel(rc.meshType), mat.tint.x, mat.tint.y, mat.tint.z);
        }
        else
        {
            DrawLine(InspectorFields::Kind::COUNT, L"render mesh=%ls (no material)", MeshLabel(rc.meshType));
        }
    }
    if (sel.collisionComponent.has_value())
    {
        const auto& cc = sel.collisionComponent.value();
        DrawLine(InspectorFields::Kind::COUNT, L"collide hx=%.2f hz=%.2f blocks=%s",
            cc.halfExtents.x, cc.halfExtents.y, cc.blocksMovement ? L"Y" : L"N");
    }
    if (sel.combatComponent.has_value())
    {
        const auto& bc = sel.combatComponent.value();
        DrawLine(InspectorFields::Kind::CombatHP, L"combat hp=%d/%d faction=%ls cd=%.2f",
            bc.health, bc.maxHealth, FactionLabel(bc.faction), bc.attackCooldownRemaining);
    }
    if (sel.enemyBehaviorComponent.has_value())
    {
        const std::wstring btype = Widen(sel.enemyBehaviorComponent->type);
        DrawLine(InspectorFields::Kind::COUNT, L"behavior type=%ls", btype.c_str());
    }
    if (sel.animationStateComponent.has_value())
    {
        const auto& a = sel.animationStateComponent.value();
        const std::wstring clip = Widen(a.clipName.empty() ? std::string("(first)") : a.clipName);
        DrawLine(InspectorFields::Kind::COUNT, L"anim clip=%ls t=%.2f", clip.c_str(), a.clipTime);
        DrawLine(InspectorFields::Kind::COUNT, L"     speed=%.2f loop=%s", a.playSpeed, a.looping ? L"Y" : L"N");
    }
    if (sel.triggerComponent.has_value())
    {
        const auto& tc = sel.triggerComponent.value();
        const std::wstring tag = Widen(tc.tag);
        DrawLine(InspectorFields::Kind::COUNT, L"trigger tag='%ls' once=%s fired=%s",
            tag.c_str(), tc.fireOnce ? L"Y" : L"N", tc.fired ? L"Y" : L"N");
        DrawLine(InspectorFields::Kind::COUNT, L"        hx=%.2f hz=%.2f", tc.halfExtents.x, tc.halfExtents.y);
        if (tc.action.has_value())
        {
            const std::wstring atype = Widen(tc.action->type);
            const std::wstring aparam = Widen(tc.action->param);
            DrawLine(InspectorFields::Kind::COUNT, L"        action=%ls param='%ls'", atype.c_str(), aparam.c_str());
        }
    }
}
