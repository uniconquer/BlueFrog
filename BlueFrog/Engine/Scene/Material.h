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
};
