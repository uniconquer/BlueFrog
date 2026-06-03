#pragma once
#include <DirectXMath.h>
#include <string>

enum class SamplerPreset
{
    WrapLinear,
    ClampLinear,
    WrapPoint,
};

struct Material
{
    std::string texturePath;                           // empty = default white
    DirectX::XMFLOAT3 tint{ 1.0f, 1.0f, 1.0f };
    SamplerPreset sampler = SamplerPreset::WrapLinear;
    // Multiplies the mesh UVs in the vertex shader. (1,1) leaves the baked UVs
    // untouched; >1 tiles the texture more (denser), <1 stretches it. Lets a
    // single seamless tile texture keep a consistent texel density across
    // planes of different world sizes (ground vs. a long thin road decal).
    DirectX::XMFLOAT2 uvScale{ 1.0f, 1.0f };
};
