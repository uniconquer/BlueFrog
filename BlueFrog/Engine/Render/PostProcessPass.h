#pragma once

#include "../../Core/Graphics.h"
#include "ConstantBuffer.h"
#include "PixelShader.h"
#include "Sampler.h"
#include "VertexShader.h"
#include <d3d11.h>
#include <wrl/client.h>

// HDR off-screen scene target + tonemapping resolve (graphics track B1).
//
// The 3D scene (lit/skinned/particles/grid/debug) renders into a
// linear R16G16B16A16_FLOAT target instead of straight to the sRGB swap
// chain. After the 3D passes, Resolve() draws a full-screen triangle that
// samples the HDR target, applies exposure + ACES filmic tonemapping, and
// writes to the sRGB back buffer (hardware sRGB-encodes on store). UI/text
// then composites on the back buffer, untouched by tonemapping.
//
// This is the foundation for later post effects (bloom samples the same HDR
// target). The HDR target shares the engine's main depth buffer so depth
// testing is identical to the no-post path.
class PostProcessPass
{
public:
	explicit PostProcessPass(Graphics& gfx);
	PostProcessPass(const PostProcessPass&) = delete;
	PostProcessPass& operator=(const PostProcessPass&) = delete;

	// Bind the HDR target (+ shared depth) and clear it. Call before the 3D
	// passes. clear color is linear.
	void BeginScene(Graphics& gfx, float r, float g, float b) noexcept;

	// Resolve the HDR target to the back buffer with exposure + ACES. Call
	// after all 3D passes and before UI/text.
	void Resolve(Graphics& gfx, float exposure) noexcept;

private:
	struct PostParams
	{
		float exposure = 1.0f;
		float pad[3]   = { 0.0f, 0.0f, 0.0f };
	};

	UINT width  = 0;
	UINT height = 0;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          pHdrTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   pHdrRtv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pHdrSrv;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  pNoDepthState;
	VertexShader                      fullscreenVS;
	PixelShader                       tonemapPS;
	Sampler                           sampler;
	PixelConstantBuffer<PostParams>   paramsBuffer;
};
