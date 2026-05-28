#pragma once

// Depth-only shaders for the shadow map pass. Two variants — static and
// skinned — mirror the LitPipeline / SkinnedPipeline input layouts so the
// same vertex buffers can be reused, but they write only SV_Position (no
// pixel shader is bound; D3D fills the depth buffer from the rasterized
// position alone).
//
// Both reuse the existing cbuffer layouts:
//   b0 TransformBuffer { matrix transform; matrix model; }
//      -> here `transform` is model * lightViewProj (not the camera VP)
//   b3 SkinningBuffer  { matrix jointMatrices[128]; }   (skinned only)
// so Renderer can upload through its existing TransformData / SkinningData
// constant buffers with the light's matrices swapped in.
namespace ShadowDepthPipeline
{
	inline const char* GetStaticShaderSource() noexcept
	{
		return
			"cbuffer TransformBuffer : register(b0)\n"
			"{\n"
			"    matrix transform;\n"
			"    matrix model;\n"
			"};\n"
			"struct VSIn\n"
			"{\n"
			"    float3 pos    : POSITION;\n"
			"    float3 normal : NORMAL;\n"
			"    float2 uv     : TEXCOORD;\n"
			"};\n"
			"float4 VSMain(VSIn input) : SV_Position\n"
			"{\n"
			"    return mul(float4(input.pos, 1.0f), transform);\n"
			"}\n";
	}

	inline const char* GetSkinnedShaderSource() noexcept
	{
		return
			"cbuffer TransformBuffer : register(b0)\n"
			"{\n"
			"    matrix transform;\n"
			"    matrix model;\n"
			"};\n"
			"cbuffer SkinningBuffer : register(b3)\n"
			"{\n"
			"    matrix jointMatrices[128];\n"
			"};\n"
			"struct VSIn\n"
			"{\n"
			"    float3 pos     : POSITION;\n"
			"    float3 normal  : NORMAL;\n"
			"    float2 uv      : TEXCOORD;\n"
			"    uint4  joints  : JOINTS;\n"
			"    float4 weights : WEIGHTS;\n"
			"};\n"
			"float4 VSMain(VSIn input) : SV_Position\n"
			"{\n"
			"    matrix skin =\n"
			"        input.weights.x * jointMatrices[input.joints.x] +\n"
			"        input.weights.y * jointMatrices[input.joints.y] +\n"
			"        input.weights.z * jointMatrices[input.joints.z] +\n"
			"        input.weights.w * jointMatrices[input.joints.w];\n"
			"    float4 skinnedPos = mul(float4(input.pos, 1.0f), skin);\n"
			"    return mul(skinnedPos, transform);\n"
			"}\n";
	}
}
