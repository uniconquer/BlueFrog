#include "UIRenderer.h"
#include "UILayout.h"
#include <algorithm>

namespace
{
	const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& GetUIInputLayoutDesc() noexcept
	{
		static const std::array<D3D11_INPUT_ELEMENT_DESC, 2> desc =
		{
			D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u,  0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			D3D11_INPUT_ELEMENT_DESC{ "COLOR",    0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
		};
		return desc;
	}

	const char* GetUIShaderSource() noexcept
	{
		// Single combined cbuffer. Previously the project carried two
		// cbuffers (Transform + Color) which produced washed-out fills;
		// merging avoids stage / register edge cases in FXC.
		return
			"cbuffer UIConstants : register(b0)\n"
			"{\n"
			"    matrix transform;\n"
			"    float3 tint;\n"
			"    float padding;\n"
			"};\n"
			"struct VSIn\n"
			"{\n"
			"    float3 pos   : POSITION;\n"
			"    float3 color : COLOR;\n"
			"};\n"
			"struct PSIn\n"
			"{\n"
			"    float4 pos   : SV_Position;\n"
			"    float3 color : COLOR;\n"
			"};\n"
			"PSIn VSMain(VSIn input)\n"
			"{\n"
			"    PSIn output;\n"
			"    output.pos   = mul(float4(input.pos, 1.0f), transform);\n"
			"    output.color = input.color;\n"
			"    return output;\n"
			"}\n"
			"float4 PSMain(PSIn input) : SV_Target\n"
			"{\n"
			"    return float4(input.color * tint, 1.0f);\n"
			"}\n";
	}
}

UIRenderer::MeshBuffers::MeshBuffers(Graphics& gfx, const Vertex* vertices, UINT vertexCount, const unsigned short* indices, UINT indexCount)
	:
	vertexBuffer(gfx, vertices, vertexCount * static_cast<UINT>(sizeof(Vertex)), sizeof(Vertex)),
	indexBuffer(gfx, indices, indexCount)
{
}

UIRenderer::UIRenderer(Graphics& gfx)
	:
	gfx(gfx),
	quadMesh(gfx, GetQuadVertices().data(), static_cast<UINT>(GetQuadVertices().size()), GetQuadIndices().data(), static_cast<UINT>(GetQuadIndices().size())),
	vertexShader(gfx, GetUIShaderSource(), "VSMain"),
	pixelShader(gfx, GetUIShaderSource(), "PSMain"),
	inputLayout(gfx, GetUIInputLayoutDesc().data(), static_cast<UINT>(GetUIInputLayoutDesc().size()), vertexShader),
	constantsBuffer(gfx),
	topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{
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

const std::array<UIRenderer::Vertex, 4>& UIRenderer::GetQuadVertices() noexcept
{
	static const std::array<Vertex, 4> vertices =
	{
		Vertex{ -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f },
		Vertex{ 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f },
		Vertex{ -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f },
		Vertex{ 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f },
	};
	return vertices;
}

const std::array<unsigned short, 6>& UIRenderer::GetQuadIndices() noexcept
{
	static const std::array<unsigned short, 6> indices =
	{
		0, 2, 1,
		2, 3, 1,
	};
	return indices;
}

void UIRenderer::BindSharedState() noexcept
{
	inputLayout.Bind(gfx);
	topology.Bind(gfx);
	vertexShader.Bind(gfx);
	// Bind the single combined cbuffer to BOTH stages at slot 0. VS reads
	// transform, PS reads tint — both from the same UIConstants struct.
	constantsBuffer.Bind(gfx); // VS slot 0 via VertexConstantBuffer base
	ID3D11Buffer* const buf = constantsBuffer.Get();
	gfx.GetContext()->PSSetConstantBuffers(0u, 1u, &buf);
	pixelShader.Bind(gfx);
}

void UIRenderer::DrawQuad(float centerX, float centerY, float width, float height, const DirectX::XMFLOAT3& tint) noexcept
{
	using namespace DirectX;

	if (width <= 0.0f || height <= 0.0f)
	{
		return;
	}

	UIConstants data = {};
	XMStoreFloat4x4(
		&data.transform,
		XMMatrixTranspose(XMMatrixScaling(width, height, 1.0f) * XMMatrixTranslation(centerX, centerY, 0.0f)));
	data.tint = tint;
	constantsBuffer.Update(gfx, data);
	quadMesh.vertexBuffer.Bind(gfx);
	quadMesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(quadMesh.indexBuffer.GetCount());
}

void UIRenderer::DrawBar(const HealthBar& bar) noexcept
{
	DrawQuad(bar.centerX, bar.centerY, bar.width, bar.height, bar.backgroundTint);

	const float clampedRatio = std::clamp(bar.ratio, 0.0f, 1.0f);
	if (clampedRatio <= 0.0f)
	{
		return;
	}

	const float fillWidth = bar.width * clampedRatio;
	const float left = bar.centerX - bar.width * 0.5f;
	DrawQuad(left + fillWidth * 0.5f, bar.centerY, fillWidth, bar.height, bar.fillTint);
}

void UIRenderer::Render(const HudState& hudState) noexcept
{
	BindSharedState();

	// Disable depth test/write for the whole UI pass so multiple quads at
	// NDC z=0 don't reject each other (panel + fill on the HP bars). Reset
	// to default after so the next frame's 3D pass starts clean.
	gfx.GetContext()->OMSetDepthStencilState(pNoDepthState.Get(), 0u);

	DrawBar(UiLayout::MakePlayerHealthBar(hudState.playerHealth.Ratio()));
	DrawBar(UiLayout::MakeAttackCooldownBar(hudState.attackCooldown01));

	if (hudState.hasTarget)
	{
		DrawBar(UiLayout::MakeTargetHealthBar(hudState.targetHealth.Ratio()));
	}

	gfx.GetContext()->OMSetDepthStencilState(nullptr, 0u);
}
