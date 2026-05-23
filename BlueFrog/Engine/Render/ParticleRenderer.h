#pragma once

#include "../../Core/Graphics.h"
#include "../Camera/TopDownCamera.h"
#include "ConstantBuffer.h"
#include "IndexBuffer.h"
#include "InputLayout.h"
#include "ParticlePipeline.h"
#include "ParticleSystem.h"
#include "PixelShader.h"
#include "Topology.h"
#include "VertexBuffer.h"
#include "VertexShader.h"
#include <DirectXMath.h>
#include <wrl/client.h>

// Draws each active particle in the supplied ParticleSystem as a
// flat alpha-blended quad on the XZ plane, colored via per-draw
// cbuffer with the particle's current age-lerped color.
//
// Same pattern as ShadowRenderer (single quad mesh, per-draw
// transform). Reuses the alpha blend setup but with depth read-only
// so particles compose over the world without overwriting depth.
class ParticleRenderer final
{
public:
	explicit ParticleRenderer(Graphics& gfx);
	ParticleRenderer(const ParticleRenderer&) = delete;
	ParticleRenderer& operator=(const ParticleRenderer&) = delete;

	void Render(const ParticleSystem& system, const TopDownCamera& camera) noexcept;

private:
	struct ParticleParams
	{
		DirectX::XMFLOAT4X4 transform;
		DirectX::XMFLOAT4   color;
	};

	Graphics&                                        gfx;
	VertexShader                                     vertexShader;
	PixelShader                                      pixelShader;
	InputLayout                                      inputLayout;
	VertexBuffer                                     vertexBuffer;
	IndexBuffer                                      indexBuffer;
	Topology                                         topology;
	VertexConstantBuffer<ParticleParams>             paramsBuffer;
	Microsoft::WRL::ComPtr<ID3D11BlendState>         pAlphaBlendState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  pDepthReadOnlyState;
};
