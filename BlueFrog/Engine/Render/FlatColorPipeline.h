#pragma once

#include <array>
#include <d3d11.h>

namespace FlatColorPipeline
{
	inline const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& GetInputLayoutDesc() noexcept
	{
		static const std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputLayoutDesc =
		{
			D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
		};
		return inputLayoutDesc;
	}

	inline const char* GetShaderSource() noexcept
	{
		return
			"cbuffer TransformBuffer : register(b0)\n"
			"{\n"
			"    matrix transform;\n"
			"};\n"
			"cbuffer ColorBuffer : register(b0)\n"
			"{\n"
			"    float3 tint;\n"
			"    float padding;\n"
			"};\n"
			"struct VSIn\n"
			"{\n"
			"    float3 pos : POSITION;\n"
			"    float3 color : COLOR;\n"
			"};\n"
			"struct PSIn\n"
			"{\n"
			"    float4 pos : SV_Position;\n"
			"    float3 color : COLOR;\n"
			"};\n"
			"PSIn VSMain(VSIn input)\n"
			"{\n"
			"    PSIn output;\n"
			"    output.pos = mul(float4(input.pos, 1.0f), transform);\n"
			"    output.color = input.color;\n"
			"    return output;\n"
			"}\n"
			"float4 PSMain(PSIn input) : SV_Target\n"
			"{\n"
			"    return float4(input.color * tint, 1.0f);\n"
			"}\n";
	}
}
