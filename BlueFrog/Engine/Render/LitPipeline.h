#pragma once

#include <array>
#include <d3d11.h>

namespace LitPipeline
{
	inline const std::array<D3D11_INPUT_ELEMENT_DESC, 3>& GetInputLayoutDesc() noexcept
	{
		static const std::array<D3D11_INPUT_ELEMENT_DESC, 3> inputLayoutDesc =
		{
			D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u,  0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "NORMAL",   0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT,    0u, 24u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
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
			"};\n"
			"struct PSIn\n"
			"{\n"
			"    float4 pos      : SV_Position;\n"
			"    float3 normalWS : NORMAL;\n"
			"    float2 uv       : TEXCOORD0;\n"
			"    float4 lightPos : TEXCOORD1;\n"
			"};\n"
			"PSIn VSMain(VSIn input)\n"
			"{\n"
			"    PSIn output;\n"
			"    float4 worldPos = mul(float4(input.pos, 1.0f), model);\n"
			"    output.pos      = mul(float4(input.pos, 1.0f), transform);\n"
			"    output.normalWS = mul(input.normal, (float3x3)model);\n"
			"    output.uv       = input.uv;\n"
			"    output.lightPos = mul(worldPos, lightViewProj);\n"
			"    return output;\n"
			"}\n"
			"float SampleShadow(float4 lightPos)\n"
			"{\n"
			"    float3 proj = lightPos.xyz / lightPos.w;\n"
			"    float2 uv   = proj.xy * float2(0.5f, -0.5f) + 0.5f;\n"
			"    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) return 1.0f;\n"
			"    float depth = proj.z - 0.0025f;\n" // depth bias to kill acne
			"    return shadowMap.SampleCmpLevelZero(shadowSampler, uv, depth);\n"
			"}\n"
			"float4 PSMain(PSIn input) : SV_Target\n"
			"{\n"
			"    float3 n        = normalize(input.normalWS);\n"
			"    float  nDotL    = saturate(dot(n, -lightDir));\n"
			"    float  shadow   = SampleShadow(input.lightPos);\n"
			"    float3 light    = ambient + nDotL * lightColor * shadow;\n"
			"    float4 albedo   = surfaceTexture.Sample(surfaceSampler, input.uv);\n"
			"    return float4(albedo.rgb * tint * light, albedo.a);\n"
			"}\n";
	}
}
