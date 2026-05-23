#include "ParticleRenderer.h"

#include <array>

namespace
{
	const std::array<ParticlePipeline::ParticleVertex, 4>& GetQuadVertices()
	{
		static const std::array<ParticlePipeline::ParticleVertex, 4> verts =
		{
			ParticlePipeline::ParticleVertex{ -1, 0, -1,  0, 0 },
			ParticlePipeline::ParticleVertex{  1, 0, -1,  1, 0 },
			ParticlePipeline::ParticleVertex{ -1, 0,  1,  0, 1 },
			ParticlePipeline::ParticleVertex{  1, 0,  1,  1, 1 },
		};
		return verts;
	}

	const std::array<unsigned short, 6>& GetQuadIndices()
	{
		static const std::array<unsigned short, 6> indices = { 0, 2, 1, 2, 3, 1 };
		return indices;
	}
}

ParticleRenderer::ParticleRenderer(Graphics& gfx_)
	:
	gfx(gfx_),
	vertexShader(gfx_, ParticlePipeline::GetShaderSource(), "VSMain"),
	pixelShader(gfx_, ParticlePipeline::GetShaderSource(), "PSMain"),
	inputLayout(gfx_, ParticlePipeline::GetInputLayoutDesc().data(),
		static_cast<UINT>(ParticlePipeline::GetInputLayoutDesc().size()),
		vertexShader),
	vertexBuffer(gfx_, GetQuadVertices().data(),
		static_cast<UINT>(GetQuadVertices().size() * sizeof(ParticlePipeline::ParticleVertex)),
		sizeof(ParticlePipeline::ParticleVertex)),
	indexBuffer(gfx_, GetQuadIndices().data(), static_cast<UINT>(GetQuadIndices().size())),
	topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST),
	paramsBuffer(gfx_)
{
	D3D11_BLEND_DESC bd = {};
	bd.RenderTarget[0].BlendEnable = TRUE;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	if (FAILED(gfx.GetDevice()->CreateBlendState(&bd, pAlphaBlendState.GetAddressOf())))
	{
		throw BFGFX_EXCEPT(E_FAIL);
	}

	D3D11_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsd.StencilEnable = FALSE;
	if (FAILED(gfx.GetDevice()->CreateDepthStencilState(&dsd, pDepthReadOnlyState.GetAddressOf())))
	{
		throw BFGFX_EXCEPT(E_FAIL);
	}
}

void ParticleRenderer::Render(const ParticleSystem& system, const TopDownCamera& camera) noexcept
{
	using namespace DirectX;
	if (system.Count() == 0) return;

	const XMMATRIX viewProj = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	vertexShader.Bind(gfx);
	pixelShader.Bind(gfx);
	inputLayout.Bind(gfx);
	vertexBuffer.Bind(gfx);
	indexBuffer.Bind(gfx);
	topology.Bind(gfx);
	paramsBuffer.Bind(gfx, 0u);

	auto* ctx = gfx.GetContext();
	const FLOAT blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	ctx->OMSetBlendState(pAlphaBlendState.Get(), blendFactor, 0xFFFFFFFF);
	ctx->OMSetDepthStencilState(pDepthReadOnlyState.Get(), 0u);

	for (const Particle& p : system.Particles())
	{
		const float t = (p.maxAge > 0.0f) ? (p.age / p.maxAge) : 1.0f;
		const float tc = std::min(std::max(t, 0.0f), 1.0f);
		XMFLOAT4 color;
		color.x = p.colorStart.x + (p.colorEnd.x - p.colorStart.x) * tc;
		color.y = p.colorStart.y + (p.colorEnd.y - p.colorStart.y) * tc;
		color.z = p.colorStart.z + (p.colorEnd.z - p.colorStart.z) * tc;
		color.w = p.colorStart.w + (p.colorEnd.w - p.colorStart.w) * tc;

		const XMMATRIX world =
			XMMatrixScaling(p.size, 1.0f, p.size) *
			XMMatrixTranslation(p.position.x, p.position.y, p.position.z);

		ParticleParams params;
		XMStoreFloat4x4(&params.transform, XMMatrixTranspose(world * viewProj));
		params.color = color;
		paramsBuffer.Update(gfx, params);

		ctx->DrawIndexed(indexBuffer.GetCount(), 0u, 0);
	}

	ctx->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
	ctx->OMSetDepthStencilState(nullptr, 0u);
}
