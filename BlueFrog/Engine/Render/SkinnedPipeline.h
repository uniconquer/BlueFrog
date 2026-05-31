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
			"    float  pad0;\n"
			"    float4 baseColorFactor;\n"
			"    float4 emissiveFactor;\n"
			"    float  metallicFactor;\n"
			"    float  roughnessFactor;\n"
			"    float  hasMetalRough;\n"
			"    float  hasNormal;\n"
			"    float  hasEmissive;\n"
			"    float  hasOcclusion;\n"
			"    float  hasAlbedo;\n"
			"    float  pbrPad;\n"
			"};\n"
			"cbuffer LightBuffer : register(b2)\n"
			"{\n"
			"    float3 lightDir;\n"
			"    float  ambient;\n"
			"    float3 lightColor;\n"
			"    float  pad1;\n"
			"    float3 camPos;\n"
			"    float  pad2;\n"
			"    float3 ambientSky;\n"
			"    float  pad3;\n"
			"    float3 ambientGround;\n"
			"    float  pad4;\n"
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
			"Texture2D metalRoughTex : register(t2);\n"
			"Texture2D normalTex     : register(t3);\n"
			"Texture2D emissiveTex   : register(t4);\n"
			"Texture2D occlusionTex  : register(t5);\n"
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
			"    float3 worldPos : TEXCOORD2;\n"
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
			"    float4 worldPos4 = mul(skinnedPos, model);\n"
			"    output.worldPos  = worldPos4.xyz;\n"
			"    output.lightPos  = mul(worldPos4, lightViewProj);\n"
			"    return output;\n"
			"}\n"
			"float SampleShadow(float4 lightPos, float nDotL)\n"
			"{\n"
			"    float3 proj = lightPos.xyz / lightPos.w;\n"
			"    float2 uv   = proj.xy * float2(0.5f, -0.5f) + 0.5f;\n"
			"    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) return 1.0f;\n"
			"    float bias  = max(0.0015f, 0.004f * (1.0f - nDotL));\n"
			"    float depth = proj.z - bias;\n"
			"    const float texel = 1.0f / 2048.0f;\n"
			"    float sum = 0.0f;\n"
			"    [unroll] for (int y = -1; y <= 1; ++y)\n"
			"        [unroll] for (int x = -1; x <= 1; ++x)\n"
			"            sum += shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2(x, y) * texel, depth);\n"
			"    return sum * (1.0f / 9.0f);\n"
			"}\n"
			// Per-pixel tangent frame from screen-space derivatives (Schuler):
			// tangent-space normal maps without a vertex TANGENT attribute.
			"float3 PerturbNormal(float3 N, float3 p, float2 uv, float3 nTex)\n"
			"{\n"
			"    float3 dp1 = ddx(p); float3 dp2 = ddy(p);\n"
			"    float2 du1 = ddx(uv); float2 du2 = ddy(uv);\n"
			"    float3 dp2perp = cross(dp2, N);\n"
			"    float3 dp1perp = cross(N, dp1);\n"
			"    float3 T = dp2perp * du1.x + dp1perp * du2.x;\n"
			"    float3 B = dp2perp * du1.y + dp1perp * du2.y;\n"
			"    float denom = max(dot(T, T), dot(B, B));\n"
			"    if (denom < 1e-12f) return N;\n"
			"    float invmax = rsqrt(denom);\n"
			"    float3x3 TBN = float3x3(T * invmax, B * invmax, N);\n"
			"    return normalize(mul(nTex, TBN));\n"
			"}\n"
			"static const float PI = 3.14159265f;\n"
			"float4 PSMain(PSIn input) : SV_Target\n"
			"{\n"
			"    float3 N = normalize(input.normalWS);\n"
			"    float3 V = normalize(camPos - input.worldPos);\n"
			"    if (hasNormal > 0.5f)\n"
			"    {\n"
			"        float3 nTex = normalTex.Sample(surfaceSampler, input.uv).xyz * 2.0f - 1.0f;\n"
			"        N = PerturbNormal(N, input.worldPos, input.uv, nTex);\n"
			"    }\n"
			"    float4 baseTex = surfaceTexture.Sample(surfaceSampler, input.uv);\n"
			"    float3 albedo  = baseTex.rgb * baseColorFactor.rgb * tint;\n"
			"    float  alpha   = baseTex.a * baseColorFactor.a;\n"
			"    float metallic, roughness;\n"
			"    if (hasMetalRough > 0.5f) {\n"
			"        float3 mr = metalRoughTex.Sample(surfaceSampler, input.uv).rgb;\n"
			"        roughness = mr.g * roughnessFactor;\n"
			"        metallic  = mr.b * metallicFactor;\n"
			"    } else { metallic = 0.0f; roughness = 1.0f; }\n"
			"    roughness = clamp(roughness, 0.045f, 1.0f);\n"
			"    float3 L = normalize(-lightDir);\n"
			"    float3 H = normalize(V + L);\n"
			"    float nDotL = saturate(dot(N, L));\n"
			"    float nDotV = saturate(dot(N, V)) + 1e-4f;\n"
			"    float nDotH = saturate(dot(N, H));\n"
			"    float vDotH = saturate(dot(V, H));\n"
			"    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);\n"
			"    float a = roughness * roughness; float a2 = a * a;\n"
			"    float dnm = nDotH * nDotH * (a2 - 1.0f) + 1.0f;\n"
			"    float D = a2 / max(PI * dnm * dnm, 1e-6f);\n"
			"    float k = (roughness + 1.0f); k = k * k / 8.0f;\n"
			"    float G = (nDotV / (nDotV * (1.0f - k) + k)) * (nDotL / (nDotL * (1.0f - k) + k));\n"
			"    float3 F = F0 + (1.0f - F0) * pow(1.0f - vDotH, 5.0f);\n"
			"    float3 spec = (D * G) * F / max(4.0f * nDotV * nDotL, 1e-4f);\n"
			"    float3 kd = (1.0f - metallic);\n"
			"    float shadow = SampleShadow(input.lightPos, nDotL);\n"
			"    float3 direct = (kd * albedo + spec) * lightColor * nDotL * shadow;\n"
			"    float ao = (hasOcclusion > 0.5f) ? occlusionTex.Sample(surfaceSampler, input.uv).r : 1.0f;\n"
			// Analytic environment reflection (lightweight IBL) — see LitPipeline.
			"    float  skyT     = N.y * 0.5f + 0.5f;\n"
			"    float3 ambLight = lerp(ambientGround, ambientSky, skyT);\n"
			"    float3 ambDiffuse = ambLight * albedo * (1.0f - metallic);\n"
			"    float3 Rdir    = reflect(-V, N);\n"
			"    float3 envSpec = lerp(ambientGround, ambientSky, Rdir.y * 0.5f + 0.5f);\n"
			"    envSpec = lerp(envSpec, ambLight, roughness);\n"
			"    float  omr  = 1.0f - roughness;\n"
			"    float3 Famb = F0 + (max(float3(omr, omr, omr), F0) - F0) * pow(1.0f - nDotV, 5.0f);\n"
			"    float3 ambientTerm = (ambDiffuse + envSpec * Famb) * ao;\n"
			"    float3 emissive = emissiveFactor.rgb;\n"
			"    if (hasEmissive > 0.5f) emissive *= emissiveTex.Sample(surfaceSampler, input.uv).rgb;\n"
			"    float3 color = direct + ambientTerm + emissive;\n"
			"    return float4(color, alpha);\n"
			"}\n";
	}
}
