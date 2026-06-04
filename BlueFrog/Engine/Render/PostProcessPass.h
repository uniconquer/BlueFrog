#pragma once

#include "../../Core/Graphics.h"
#include "ConstantBuffer.h"
#include "PixelShader.h"
#include "Sampler.h"
#include "VertexShader.h"
#include <d3d11.h>
#include <wrl/client.h>

// HDR off-screen scene target + bloom + tonemapping resolve (graphics B1/B3).
//
// The 3D scene renders into a linear R16G16B16A16_FLOAT target instead of
// straight to the sRGB swap chain. After the 3D passes, Resolve():
//   1. bright-pass: extract HDR pixels above a threshold into a half-res target
//   2. separable Gaussian blur (H then V) on that target
//   3. composite: full-screen triangle adds the blurred bloom to the scene,
//      applies exposure + ACES tonemapping, and writes the sRGB back buffer
// UI/text then composite on the back buffer, untouched by tonemapping.
//
// The HDR target shares the engine's main depth buffer so depth testing is
// identical to the no-post path.
class PostProcessPass
{
public:
	explicit PostProcessPass(Graphics& gfx);
	PostProcessPass(const PostProcessPass&) = delete;
	PostProcessPass& operator=(const PostProcessPass&) = delete;

	// Bind the HDR target (+ shared depth) and clear it. Call before the 3D
	// passes. clear color is linear.
	void BeginScene(Graphics& gfx, float r, float g, float b) noexcept;

	// Run bloom + resolve the HDR target to the back buffer with exposure +
	// ACES. Call after all 3D passes and before UI/text.
	void Resolve(Graphics& gfx, float exposure, float saturation = 1.0f, float contrast = 1.0f) noexcept;

private:
	struct PostParams
	{
		float exposure       = 1.0f;
		float bloomThreshold = 0.8f;
		float bloomIntensity = 0.7f;
		float saturation     = 1.0f; // composite vibrance (was pad0)
		float blurDirX       = 0.0f;
		float blurDirY       = 0.0f;
		float contrast       = 1.0f; // composite S-curve strength (was pad1)
		float pad2           = 0.0f;
	};

	// Bind one off-screen RTV at the given size, no depth.
	void SetTarget(Graphics& gfx, ID3D11RenderTargetView* rtv, UINT w, UINT h) noexcept;

	UINT width  = 0;
	UINT height = 0;
	UINT bloomW = 0;
	UINT bloomH = 0;

	Microsoft::WRL::ComPtr<ID3D11Texture2D>          pHdrTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   pHdrRtv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pHdrSrv;
	// Two half-res ping-pong targets for bright-pass + separable blur.
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          pBloomTex[2];
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   pBloomRtv[2];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pBloomSrv[2];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  pNoDepthState;

	VertexShader                    fullscreenVS;
	PixelShader                     tonemapPS;    // composite scene + bloom, exposure, ACES
	PixelShader                     brightPassPS; // threshold extract
	PixelShader                     blurPS;       // separable Gaussian
	Sampler                         sampler;
	PixelConstantBuffer<PostParams> paramsBuffer;
};
