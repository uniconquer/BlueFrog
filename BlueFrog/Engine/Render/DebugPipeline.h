#pragma once

#include <array>
#include <d3d11.h>

namespace DebugPipeline
{
    struct DebugVertex
    {
        float x, y, z;
        float r, g, b;
    };

    inline const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& GetInputLayoutDesc() noexcept
    {
        static const std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputLayoutDesc =
        {
            D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u,  0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
            D3D11_INPUT_ELEMENT_DESC{ "COLOR",    0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
        };
        return inputLayoutDesc;
    }

    // Pos+Color line shader. No lighting, no model matrix — vertices are
    // already in world space (the renderer bakes object transforms into the
    // line vertices it appends), so the shader only applies the camera's
    // view-projection. Pixel shader passes the interpolated color through.
    inline const char* GetShaderSource() noexcept
    {
        return
            "cbuffer DebugTransformBuffer : register(b0)\n"
            "{\n"
            "    matrix viewProj;\n"
            "};\n"
            "struct VSIn\n"
            "{\n"
            "    float3 pos   : POSITION;\n"
            "    float3 color : COLOR;\n"
            "};\n"
            "struct PSIn\n"
            "{\n"
            "    float4 pos   : SV_Position;\n"
            "    float3 color : COLOR;\n"
            "};\n"
            "PSIn VSMain(VSIn input)\n"
            "{\n"
            "    PSIn output;\n"
            "    output.pos   = mul(float4(input.pos, 1.0f), viewProj);\n"
            "    output.color = input.color;\n"
            "    return output;\n"
            "}\n"
            "float4 PSMain(PSIn input) : SV_Target\n"
            "{\n"
            "    return float4(input.color, 1.0f);\n"
            "}\n";
    }
}
