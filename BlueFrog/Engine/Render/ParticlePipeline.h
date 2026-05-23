#pragma once

#include <array>
#include <d3d11.h>

// Particle pipeline: small flat XZ quad per particle, color uniform
// per-particle via a tiny per-draw cbuffer. Alpha blended ("over"
// compositing) for soft edges. No texture — color + alpha is enough
// for v1 (hit splashes, sparkles).
namespace ParticlePipeline
{
	struct ParticleVertex
	{
		float x, y, z;
		float u, v;
	};

	inline const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& GetInputLayoutDesc() noexcept
	{
		static const std::array<D3D11_INPUT_ELEMENT_DESC, 2> desc =
		{
			D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u,  0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT,    0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
		};
		return desc;
	}

	inline const char* GetShaderSource() noexcept
	{
		// Color forwarded VS→PS via interpolant, same trick as ShadowPipeline,
		// so the PS doesn't need a separate cbuffer binding.
		return
			"cbuffer ParticleParams : register(b0)\n"
			"{\n"
			"    matrix transform;\n"
			"    float4 color;\n"
			"};\n"
			"struct VSIn { float3 pos : POSITION; float2 uv : TEXCOORD; };\n"
			"struct PSIn {\n"
			"    float4 pos : SV_Position;\n"
			"    float2 uv  : TEXCOORD0;\n"
			"    float4 col : COLOR0;\n"
			"};\n"
			"PSIn VSMain(VSIn i) {\n"
			"    PSIn o;\n"
			"    o.pos = mul(float4(i.pos, 1.0f), transform);\n"
			"    o.uv  = i.uv;\n"
			"    o.col = color;\n"
			"    return o;\n"
			"}\n"
			"float4 PSMain(PSIn i) : SV_Target {\n"
			"    float2 c = i.uv * 2.0f - 1.0f;\n"
			"    float  d = saturate(length(c));\n"
			"    float  falloff = 1.0f - d;\n"
			"    falloff = falloff * falloff;\n"  // soft round
			"    return float4(i.col.rgb, i.col.a * falloff);\n"
			"}\n";
	}
}
