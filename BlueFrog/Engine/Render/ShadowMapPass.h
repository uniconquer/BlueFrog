#pragma once

#include "../../Core/Graphics.h"
#include <d3d11.h>
#include <wrl/client.h>

// Off-screen depth target for single-directional-light shadow mapping.
//
// Owns a square depth texture rendered from the sun's point of view. The
// scene's shadow casters are drawn into it (depth only, no color) by the
// Renderer; the main lit/skinned passes then sample it with a comparison
// sampler to decide whether each pixel is in shadow.
//
// This class deliberately does NOT know how to draw the scene — it only
// manages the depth target + the comparison sampler. The Renderer owns the
// depth-only shaders and the caster loop so that joint-matrix computation
// can be shared between the depth pass and the main pass (skinned meshes
// must pose identically in both, or the shadow won't line up with the
// model).
//
// Resolution note: 2048² single cascade. Top-down cameras see a small
// slice of the world, so one tight ortho frustum that follows the camera
// gives crisp shadows without the complexity of cascaded shadow maps.
class ShadowMapPass
{
public:
	static constexpr UINT kSize = 2048;

	explicit ShadowMapPass(Graphics& gfx);
	ShadowMapPass(const ShadowMapPass&) = delete;
	ShadowMapPass& operator=(const ShadowMapPass&) = delete;

	// Bind the depth texture as the render target (no color target),
	// set the shadow-map viewport, and clear depth to 1.0. After this the
	// Renderer issues depth-only draws for every caster.
	void Begin(Graphics& gfx) noexcept;

	// Unbind the depth texture from the output stage so it can be bound
	// as a shader resource in the main pass. (The caller is responsible
	// for calling Graphics::RestoreBackBuffer afterwards to return to the
	// main scene target + viewport.)
	void End(Graphics& gfx) noexcept;

	// Bind / unbind the shadow map + comparison sampler for the main pass.
	// slot/samplerSlot match the t1/s1 registers used by the lit + skinned
	// shaders.
	void BindForRead(Graphics& gfx, UINT srvSlot, UINT samplerSlot) noexcept;
	void UnbindForRead(Graphics& gfx, UINT srvSlot) noexcept;

private:
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   pDsv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pSrv;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>       pComparisonSampler;
};
