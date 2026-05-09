#include "WorldGridRenderer.h"

#include <vector>

namespace
{
	// Tunables. Half-extent of 20 = ±20m grid, 41 lines per axis.
	constexpr int   kHalfExtent       = 20;
	constexpr int   kMajorEvery       = 5;
	constexpr float kMinorBrightness  = 0.18f;
	constexpr float kMajorBrightness  = 0.40f;
	constexpr float kY                = 0.005f; // tiny lift to avoid z-fight w/ ground when depth re-enables

	using V = DebugPipeline::DebugVertex;

	void EmitLine(std::vector<V>& out,
		float x0, float z0, float x1, float z1,
		float r, float g, float b)
	{
		out.push_back({ x0, kY, z0, r, g, b });
		out.push_back({ x1, kY, z1, r, g, b });
	}

	std::vector<V> BuildGrid()
	{
		std::vector<V> verts;
		// Minor + major grid lines. Lines parallel to X axis (varying Z).
		for (int i = -kHalfExtent; i <= kHalfExtent; ++i)
		{
			const bool major = (i % kMajorEvery) == 0;
			const bool isAxis = (i == 0);
			float r = major ? kMajorBrightness : kMinorBrightness;
			float g = major ? kMajorBrightness : kMinorBrightness;
			float b = major ? kMajorBrightness : kMinorBrightness;
			if (isAxis)
			{
				// The X axis line itself runs along z=0; make it red. The
				// other "axis" tag (z line at x=0) is handled below.
				r = 0.85f; g = 0.18f; b = 0.18f;
			}
			EmitLine(verts,
				-static_cast<float>(kHalfExtent), static_cast<float>(i),
				 static_cast<float>(kHalfExtent), static_cast<float>(i),
				r, g, b);
		}
		// Lines parallel to Z axis (varying X).
		for (int i = -kHalfExtent; i <= kHalfExtent; ++i)
		{
			const bool major = (i % kMajorEvery) == 0;
			const bool isAxis = (i == 0);
			float r = major ? kMajorBrightness : kMinorBrightness;
			float g = major ? kMajorBrightness : kMinorBrightness;
			float b = major ? kMajorBrightness : kMinorBrightness;
			if (isAxis)
			{
				// Z axis (x=0): blue.
				r = 0.18f; g = 0.36f; b = 0.95f;
			}
			EmitLine(verts,
				static_cast<float>(i), -static_cast<float>(kHalfExtent),
				static_cast<float>(i),  static_cast<float>(kHalfExtent),
				r, g, b);
		}
		return verts;
	}
}

WorldGridRenderer::WorldGridRenderer(Graphics& gfxIn)
	:
	gfx(gfxIn),
	vertexShader(gfx, DebugPipeline::GetShaderSource(), "VSMain"),
	pixelShader (gfx, DebugPipeline::GetShaderSource(), "PSMain"),
	inputLayout (gfx, DebugPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(DebugPipeline::GetInputLayoutDesc().size()), vertexShader),
	viewProjBuffer(gfx),
	topologyLineList(D3D11_PRIMITIVE_TOPOLOGY_LINELIST)
{
	const auto verts = BuildGrid();
	vertexCount = static_cast<UINT>(verts.size());

	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags          = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage              = D3D11_USAGE_IMMUTABLE;
	desc.CPUAccessFlags     = 0u;
	desc.ByteWidth          = vertexCount * static_cast<UINT>(sizeof(V));
	desc.StructureByteStride = sizeof(V);

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = verts.data();

	if (const HRESULT hr = gfx.GetDevice()->CreateBuffer(&desc, &init, pVertexBuffer.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}

	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable    = FALSE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc      = D3D11_COMPARISON_ALWAYS;
	dsDesc.StencilEnable  = FALSE;
	if (const HRESULT hr = gfx.GetDevice()->CreateDepthStencilState(&dsDesc, pNoDepthState.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}

void WorldGridRenderer::Render(const TopDownCamera& camera) noexcept
{
	using namespace DirectX;

	if (vertexCount == 0u || !pVertexBuffer) return;

	vertexShader.Bind(gfx);
	pixelShader.Bind(gfx);
	inputLayout.Bind(gfx);
	topologyLineList.Bind(gfx);

	const XMMATRIX viewProj = camera.GetViewMatrix() * camera.GetProjectionMatrix();
	ViewProjData vpData = {};
	XMStoreFloat4x4(&vpData.viewProj, XMMatrixTranspose(viewProj));
	viewProjBuffer.Update(gfx, vpData);
	viewProjBuffer.Bind(gfx, 0u);

	const UINT stride = sizeof(V);
	const UINT offset = 0u;
	ID3D11Buffer* const buffer = pVertexBuffer.Get();
	gfx.GetContext()->IASetVertexBuffers(0u, 1u, &buffer, &stride, &offset);

	gfx.GetContext()->OMSetDepthStencilState(pNoDepthState.Get(), 0u);
	gfx.GetContext()->Draw(vertexCount, 0u);
	gfx.GetContext()->OMSetDepthStencilState(nullptr, 0u);
}
