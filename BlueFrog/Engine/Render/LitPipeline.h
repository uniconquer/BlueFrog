#pragma once

#include <array>
#include <d3d11.h>

namespace LitPipeline
{
	inline const std::array<D3D11_INPUT_ELEMENT_DESC, 4>& GetInputLayoutDesc() noexcept
	{
		static const std::array<D3D11_INPUT_ELEMENT_DESC, 4> inputLayoutDesc =
		{
			D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT,    0u,  0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "NORMAL",   0u, DXGI_FORMAT_R32G32B32_FLOAT,    0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT,       0u, 24u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "COLOR",    0u, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, 32u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
		};
		return inputLayoutDesc;
	}

	inline const char* GetShaderSource() noexcept
	{
		return
			"cbuffer TransformBuffer : register(b0)\n"
			"{\n"
			"    matrix transform;\n"
			"    matrix model;\n"
			"};\n"
			"cbuffer MaterialBuffer : register(b1)\n"
			"{\n"
			"    float3 tint;\n"
			"    float pad0;\n"
			"};\n"
			"cbuffer LightBuffer : register(b2)\n"
			"{\n"
			"    float3 lightDir;\n"
			"    float  ambient;\n"
			"    float3 lightColor;\n"
			"    float  pad1;\n"
			"};\n"
			"cbuffer ShadowBuffer : register(b4)\n"
			"{\n"
			"    matrix lightViewProj;\n"
			"};\n"
			"Texture2D surfaceTexture : register(t0);\n"
			"SamplerState surfaceSampler : register(s0);\n"
			"Texture2D shadowMap : register(t1);\n"
			"SamplerComparisonState shadowSampler : register(s1);\n"
			"struct VSIn\n"
			"{\n"
			"    float3 pos    : POSITION;\n"
			"    float3 normal : NORMAL;\n"
			"    float2 uv     : TEXCOORD;\n"
			"    float4 color  : COLOR;\n"
			"};\n"
			"struct PSIn\n"
			"{\n"
			"    float4 pos      : SV_Position;\n"
			"    float3 normalWS : NORMAL;\n"
			"    float2 uv       : TEXCOORD0;\n"
			"    float4 lightPos : TEXCOORD1;\n"
			"    float4 color    : COLOR;\n"
			"};\n"
			"PSIn VSMain(VSIn input)\n"
			"{\n"
			"    PSIn output;\n"
			"    float4 worldPos = mul(float4(input.pos, 1.0f), model);\n"
			"    output.pos      = mul(float4(input.pos, 1.0f), transform);\n"
			"    output.normalWS = mul(input.normal, (float3x3)model);\n"
			"    output.uv       = input.uv;\n"
			"    output.lightPos = mul(worldPos, lightViewProj);\n"
			"    output.color    = input.color;\n"
			"    return output;\n"
			"}\n"
			"float SampleShadow(float4 lightPos, float nDotL)\n"
			"{\n"
			"    float3 proj = lightPos.xyz / lightPos.w;\n"
			"    float2 uv   = proj.xy * float2(0.5f, -0.5f) + 0.5f;\n"
			"    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) return 1.0f;\n"
			// Slope-scaled bias: grazing faces (small nDotL) self-shadow and
			// need more bias; near-flat-lit faces need little, keeping the
			// contact line tight instead of peter-panning.
			"    float bias  = max(0.0015f, 0.004f * (1.0f - nDotL));\n"
			"    float depth = proj.z - bias;\n"
			// 3x3 PCF. Each tap is itself hardware-bilinear (comparison
			// sampler), so this softens edges well beyond a single tap.
			// texel must match ShadowMapPass::kSize (2048).
			"    const float texel = 1.0f / 2048.0f;\n"
			"    float sum = 0.0f;\n"
			"    [unroll] for (int y = -1; y <= 1; ++y)\n"
			"        [unroll] for (int x = -1; x <= 1; ++x)\n"
			"            sum += shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2(x, y) * texel, depth);\n"
			"    return sum * (1.0f / 9.0f);\n"
			"}\n"
			"float4 PSMain(PSIn input) : SV_Target\n"
			"{\n"
			"    float3 n        = normalize(input.normalWS);\n"
			"    float  nDotL    = saturate(dot(n, -lightDir));\n"
			"    float  shadow   = SampleShadow(input.lightPos, nDotL);\n"
			"    float3 light    = ambient + nDotL * lightColor * shadow;\n"
			"    float4 albedo   = surfaceTexture.Sample(surfaceSampler, input.uv);\n"
			"    return float4(albedo.rgb * input.color.rgb * tint * light, albedo.a * input.color.a);\n"
			"}\n";
	}
}
