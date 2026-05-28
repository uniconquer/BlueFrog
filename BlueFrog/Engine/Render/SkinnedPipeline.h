#pragma once

#include <array>
#include <d3d11.h>

namespace SkinnedPipeline
{
	// Compile-time max joints per skinned mesh. 128 covers the Mixamo
	// default rig (65 joints) with comfortable headroom for facial /
	// fingered variants and the occasional weapon socket bone. D3D11
	// cbuffers cap at 64KB so we have plenty of room (128 * 64B = 8KB).
	// If you bump this further, the HLSL literal below has to match.
	inline constexpr int MaxJoints = 128;

	// Vertex format consumed by the skinned shader. Stride 56 bytes:
	//   12 (pos) + 12 (normal) + 8 (uv) + 8 (joints u16x4) + 16 (weights f32x4).
	struct SkinnedVertex
	{
		float       x, y, z;
		float       nx, ny, nz;
		float       u, v;
		std::uint16_t j0, j1, j2, j3;
		float       w0, w1, w2, w3;
	};

	inline const std::array<D3D11_INPUT_ELEMENT_DESC, 5>& GetInputLayoutDesc() noexcept
	{
		static const std::array<D3D11_INPUT_ELEMENT_DESC, 5> inputLayoutDesc =
		{
			D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT,    0u,  0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "NORMAL",   0u, DXGI_FORMAT_R32G32B32_FLOAT,    0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT,       0u, 24u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "JOINTS",   0u, DXGI_FORMAT_R16G16B16A16_UINT,  0u, 32u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "WEIGHTS",  0u, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, 40u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
		};
		return inputLayoutDesc;
	}

	// Skin VS + lit PS. The PS is functionally the same as LitPipeline's,
	// duplicated here so each pipeline owns its full shader text and we
	// don't introduce a fragile cross-pipeline include order.
	//
	// Skinning math: weighted sum of (jointMatrix * vertex). For a 4-influence
	// vertex with normalized weights, we compute:
	//   skin = w0*M[j0] + w1*M[j1] + w2*M[j2] + w3*M[j3]
	//   skinnedPos    = skin * float4(pos, 1)
	//   skinnedNormal = (float3x3)skin * normal   (assumes uniform scale)
	// Then the standard MVP / model transform finishes the job.
	//
	// At Stage 2 the renderer always uploads identity matrices (bind pose).
	// Stage 3 will compute jointMatrix[i] = animatedJointWorld * inverseBindMatrix
	// per frame; the shader code does not change.
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
			"cbuffer SkinningBuffer : register(b3)\n"
			"{\n"
			"    matrix jointMatrices[128];\n"
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
			"    float3 pos     : POSITION;\n"
			"    float3 normal  : NORMAL;\n"
			"    float2 uv      : TEXCOORD;\n"
			"    uint4  joints  : JOINTS;\n"
			"    float4 weights : WEIGHTS;\n"
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
			"    matrix skin =\n"
			"        input.weights.x * jointMatrices[input.joints.x] +\n"
			"        input.weights.y * jointMatrices[input.joints.y] +\n"
			"        input.weights.z * jointMatrices[input.joints.z] +\n"
			"        input.weights.w * jointMatrices[input.joints.w];\n"
			"    float4 skinnedPos    = mul(float4(input.pos, 1.0f), skin);\n"
			"    float3 skinnedNormal = mul(input.normal, (float3x3)skin);\n"
			"    output.pos      = mul(skinnedPos, transform);\n"
			"    output.normalWS = mul(skinnedNormal, (float3x3)model);\n"
			"    output.uv       = input.uv;\n"
			"    output.lightPos = mul(mul(skinnedPos, model), lightViewProj);\n"
			"    return output;\n"
			"}\n"
			"float SampleShadow(float4 lightPos)\n"
			"{\n"
			"    float3 proj = lightPos.xyz / lightPos.w;\n"
			"    float2 uv   = proj.xy * float2(0.5f, -0.5f) + 0.5f;\n"
			"    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) return 1.0f;\n"
			"    float depth = proj.z - 0.0025f;\n"
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
