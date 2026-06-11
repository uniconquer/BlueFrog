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
			"    float  ghostAlpha;\n"
			"    float2 uvScale;\n"
			"    float2 matPad;\n"
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
			"    float3 cutoutTarget;\n"
			"    float  cutoutRadius;\n"
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
			"    float3 worldPos : TEXCOORD2;\n"
			"    float4 color    : COLOR;\n"
			"};\n"
			"PSIn VSMain(VSIn input)\n"
			"{\n"
			"    PSIn output;\n"
			"    float4 worldPos = mul(float4(input.pos, 1.0f), model);\n"
			"    output.pos      = mul(float4(input.pos, 1.0f), transform);\n"
			"    output.normalWS = mul(input.normal, (float3x3)model);\n"
			"    output.uv       = input.uv;\n" // uvScale applied in PS (b1 is PS-only)
			"    output.lightPos = mul(worldPos, lightViewProj);\n"
			"    output.worldPos = worldPos.xyz;\n"
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
			"    float bias  = max(0.0018f, 0.005f * (1.0f - nDotL));\n"
			"    float depth = proj.z - bias;\n"
			// 5x5 PCF (Shadow S4). Each tap is itself hardware-bilinear
			// (comparison sampler) so the penumbra spans ~6 texels — soft but
			// still tight since the ortho frustum shrank to 28u. The slope
			// bias is a touch higher than the old 3x3's because the outer
			// taps land farther from the receiver. texel must match
			// ShadowMapPass::kSize (2048).
			"    const float texel = 1.0f / 2048.0f;\n"
			"    float sum = 0.0f;\n"
			"    [unroll] for (int y = -2; y <= 2; ++y)\n"
			"        [unroll] for (int x = -2; x <= 2; ++x)\n"
			"            sum += shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2(x, y) * texel, depth);\n"
			"    return sum * (1.0f / 25.0f);\n"
			"}\n"
			// Per-pixel tangent frame from screen-space derivatives (Schuler).
			// Lets us apply a tangent-space normal map without a vertex TANGENT
			// attribute -- the helmet (and most kit assets) ship none.
			"float3 PerturbNormal(float3 N, float3 p, float2 uv, float3 nTex)\n"
			"{\n"
			"    float3 dp1 = ddx(p); float3 dp2 = ddy(p);\n"
			"    float2 du1 = ddx(uv); float2 du2 = ddy(uv);\n"
			"    float3 dp2perp = cross(dp2, N);\n"
			"    float3 dp1perp = cross(N, dp1);\n"
			"    float3 T = dp2perp * du1.x + dp1perp * du2.x;\n"
			"    float3 B = dp2perp * du1.y + dp1perp * du2.y;\n"
			// Guard against zero UV gradient (UV-less meshes): rsqrt(0)=inf
			// would make T/B NaN and poison the normal. Fall back to geometric N.
			"    float denom = max(dot(T, T), dot(B, B));\n"
			"    if (denom < 1e-12f) return N;\n"
			"    float invmax = rsqrt(denom);\n"
			"    float3x3 TBN = float3x3(T * invmax, B * invmax, N);\n"
			"    return normalize(mul(nTex, TBN));\n"
			"}\n"
			"static const float PI = 3.14159265f;\n"
			"float4 PSMain(PSIn input) : SV_Target\n"
			"{\n"
			// Camera cutout: dither-discard fragments inside the eye->target
			// cylinder so the player reads through roofs/walls. Stays on the
			// opaque pipeline (depth writes intact) — per-PIXEL holes instead
			// of per-object alpha, so stacked house modules can't double-blend
			// into the layered-translucency mush the ghost fade produced.
			// Radius 0 disables (shadow/depth passes use separate shaders and
			// are unaffected: cut pixels still cast shadows, which grounds the
			// building visually).
			"    if (cutoutRadius > 0.0f)\n"
			"    {\n"
			"        float3 cray = cutoutTarget - camPos;\n"
			"        float  ct   = dot(input.worldPos - camPos, cray) / max(dot(cray, cray), 1e-4f);\n"
			"        if (ct > 0.02f && ct < 0.93f)\n"
			"        {\n"
			"            float cd = length(input.worldPos - (camPos + cray * ct));\n"
			"            if (cd < cutoutRadius)\n"
			"            {\n"
			"                float coverage = saturate(cd / cutoutRadius);\n"
			"                coverage *= coverage;\n"
			"                const float bayer[16] = { 0.0f, 8.0f, 2.0f, 10.0f, 12.0f, 4.0f, 14.0f, 6.0f, 3.0f, 11.0f, 1.0f, 9.0f, 15.0f, 7.0f, 13.0f, 5.0f };\n"
			"                uint2 px = uint2(input.pos.xy) & 3u;\n"
			"                if (coverage < (bayer[px.y * 4u + px.x] + 0.5f) / 16.0f) discard;\n"
			"            }\n"
			"        }\n"
			"    }\n"
			"    float3 N = normalize(input.normalWS);\n"
			"    float3 V = normalize(camPos - input.worldPos);\n"
			// Tiling lives here (PS) not the VS: the MaterialBuffer (b1) is bound
			// to the pixel shader only, so reading uvScale in the VS gave 0 and
			// collapsed every textured surface to a flat average color.
			"    float2 uv = input.uv * uvScale;\n"
			// Normal map: always sampled (default white bound when absent) and
			// blended in by the hasNormal flag so the ddx/ddy stays uniform.
			// Uniform branch (hasNormal is a per-draw constant) so the ddx/ddy
			// inside stay legal and UV-less meshes skip the map entirely.
			"    if (hasNormal > 0.5f)\n"
			"    {\n"
			"        float3 nTex = normalTex.Sample(surfaceSampler, uv).xyz * 2.0f - 1.0f;\n"
			"        N = PerturbNormal(N, input.worldPos, uv, nTex);\n"
			"    }\n"
			// Albedo (sRGB SRV -> linear) * factors * vertex color * tint.
			"    float4 baseTex = surfaceTexture.Sample(surfaceSampler, uv);\n"
			"    float3 albedo  = baseTex.rgb * baseColorFactor.rgb * input.color.rgb * tint;\n"
			"    float  alpha   = baseTex.a * baseColorFactor.a * input.color.a;\n"
			// Metallic-roughness. With no MR map, force a rough dielectric so
			// stylized kit assets stay ~diffuse (glTF's metallic default is 1,
			// which would otherwise turn every untextured wall to metal).
			"    float metallic, roughness;\n"
			"    if (hasMetalRough > 0.5f) {\n"
			"        float3 mr = metalRoughTex.Sample(surfaceSampler, uv).rgb;\n"
			"        roughness = mr.g * roughnessFactor;\n"
			"        metallic  = mr.b * metallicFactor;\n"
			"    } else { metallic = 0.0f; roughness = 1.0f; }\n"
			"    roughness = clamp(roughness, 0.045f, 1.0f);\n"
			// Cook-Torrance direct lighting from the single directional sun.
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
			// kd: diffuse weight, killed for metals. We deliberately drop the
			// (1-F) Fresnel factor here so non-PBR / vertex-colored assets keep
			// the old flat-Lambert brightness (the energy loss is negligible
			// for the stylized look and avoids darkening tree silhouettes). No
			// 1/PI either, for the same reason.
			"    float3 kd = (1.0f - metallic);\n"
			"    float shadow = SampleShadow(input.lightPos, nDotL);\n"
			"    float3 direct = (kd * albedo + spec) * lightColor * nDotL * shadow;\n"
			// Ambient (no IBL): flat term on diffuse albedo + a crude specular
			// floor on F0 so metals aren't pure black. Modulated by AO.
			"    float ao = (hasOcclusion > 0.5f) ? occlusionTex.Sample(surfaceSampler, uv).r : 1.0f;\n"
			// Analytic environment (lightweight IBL). Diffuse irradiance is the
			// hemispheric fill at N; specular is the sky color along the
			// reflection vector, blurred toward that fill by roughness and
			// weighted by a roughness-aware Fresnel. Metals pick up a
			// sky-tinted reflection that shifts with surface orientation.
			"    float  skyT     = N.y * 0.5f + 0.5f;\n"
			"    float3 ambLight = lerp(ambientGround, ambientSky, skyT);\n"
			"    float3 ambDiffuse = ambLight * albedo * (1.0f - metallic);\n"
			"    float3 Rdir    = reflect(-V, N);\n"
			"    float3 envSpec = lerp(ambientGround, ambientSky, Rdir.y * 0.5f + 0.5f);\n"
			"    envSpec = lerp(envSpec, ambLight, roughness);\n"
			"    float  omr  = 1.0f - roughness;\n"
			"    float3 Famb = F0 + (max(float3(omr, omr, omr), F0) - F0) * pow(1.0f - nDotV, 5.0f);\n"
			"    float3 ambientTerm = (ambDiffuse + envSpec * Famb) * ao;\n"
			// Emissive: factor defaults to 0 for non-emissive materials.
			"    float3 emissive = emissiveFactor.rgb;\n"
			"    if (hasEmissive > 0.5f) emissive *= emissiveTex.Sample(surfaceSampler, uv).rgb;\n"
			"    float3 color = direct + ambientTerm + emissive;\n"
			"    return float4(color, alpha * ghostAlpha);\n"
			"}\n";
	}
}
