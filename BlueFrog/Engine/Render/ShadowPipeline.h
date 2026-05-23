#pragma once

#include <array>
#include <d3d11.h>

// Blob-shadow pipeline: a single textured quad per shadow caster,
// alpha-blended onto whatever is below it. The "texture" is procedural
// — the pixel shader computes a soft circular falloff from the UV,
// so no asset file needs to ship for the v1 shadow effect.
//
// Pipeline shape:
//   VS: model→view→projection transform of a -1..+1 quad in XZ plane.
//   PS: distance-from-center falloff × global alpha → 0..N alpha,
//        RGB always (0,0,0). Outputs straight black with varying alpha.
namespace ShadowPipeline
{
	struct ShadowVertex
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
		// Alpha is forwarded VS→PS through an interpolant so the PS
		// doesn't need its own cbuffer binding — keeps the pipeline
		// to a single VS cbuffer slot.
		return
			"cbuffer ShadowParams : register(b0)\n"
			"{\n"
			"    matrix transform;\n"
			"    float  shadowAlpha;\n"
			"    float3 pad;\n"
			"};\n"
			"struct VSIn { float3 pos : POSITION; float2 uv : TEXCOORD; };\n"
			"struct PSIn {\n"
			"    float4 pos : SV_Position;\n"
			"    float2 uv  : TEXCOORD0;\n"
			"    float  a   : TEXCOORD1;\n"
			"};\n"
			"PSIn VSMain(VSIn i) {\n"
			"    PSIn o;\n"
			"    o.pos = mul(float4(i.pos, 1.0f), transform);\n"
			"    o.uv  = i.uv;\n"
			"    o.a   = shadowAlpha;\n"
			"    return o;\n"
			"}\n"
			"float4 PSMain(PSIn i) : SV_Target {\n"
			"    float2 c = i.uv * 2.0f - 1.0f;\n"   // [-1, 1]
			"    float  d = saturate(length(c));\n"
			"    float  falloff = 1.0f - d;\n"
			"    falloff = falloff * falloff;\n"
			"    return float4(0.0f, 0.0f, 0.0f, falloff * i.a);\n"
			"}\n";
	}
}
