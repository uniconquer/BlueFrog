#include "UIRenderer.h"
#include <algorithm>

using namespace DirectX;

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
	vertexShader(gfx, GetShaderSource(), "VSMain"),
	pixelShader(gfx, GetShaderSource(), "PSMain"),
	inputLayout(gfx, GetInputLayoutDesc().data(), static_cast<UINT>(GetInputLayoutDesc().size()), vertexShader),
	transformBuffer(gfx),
	colorBuffer(gfx),
	topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{
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

const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& UIRenderer::GetInputLayoutDesc() noexcept
{
	static const std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputLayoutDesc =
	{
		D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
		D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
	};
	return inputLayoutDesc;
}

const char* UIRenderer::GetShaderSource() noexcept
{
	return
		"cbuffer TransformBuffer : register(b0)\n"
		"{\n"
		"    matrix transform;\n"
		"};\n"
		"cbuffer ColorBuffer : register(b0)\n"
		"{\n"
		"    float3 tint;\n"
		"    float padding;\n"
		"};\n"
		"struct VSIn\n"
		"{\n"
		"    float3 pos : POSITION;\n"
		"    float3 color : COLOR;\n"
		"};\n"
		"struct PSIn\n"
		"{\n"
		"    float4 pos : SV_Position;\n"
		"    float3 color : COLOR;\n"
		"};\n"
		"PSIn VSMain(VSIn input)\n"
		"{\n"
		"    PSIn output;\n"
		"    output.pos = mul(float4(input.pos, 1.0f), transform);\n"
		"    output.color = input.color;\n"
		"    return output;\n"
		"}\n"
		"float4 PSMain(PSIn input) : SV_Target\n"
		"{\n"
		"    return float4(input.color * tint, 1.0f);\n"
		"}\n";
}

void UIRenderer::BindSharedState() noexcept
{
	inputLayout.Bind(gfx);
	topology.Bind(gfx);
	vertexShader.Bind(gfx);
	transformBuffer.Bind(gfx);
	colorBuffer.Bind(gfx);
	pixelShader.Bind(gfx);
}

void UIRenderer::DrawQuad(const HealthBar& bar, float ratio) noexcept
{
	if (!bar.visible)
	{
		return;
	}

	const float clampedRatio = std::clamp(ratio, 0.0f, 1.0f);
	const float width = bar.size.x;
	const float height = bar.size.y;

	TransformData transformData = {};
	XMStoreFloat4x4(
		&transformData.transform,
		XMMatrixTranspose(XMMatrixScaling(width, height, 1.0f) * XMMatrixTranslation(bar.center.x, bar.center.y, 0.0f)));

	const ColorData backgroundColor = { bar.backgroundTint, 0.0f };
	transformBuffer.Update(gfx, transformData);
	colorBuffer.Update(gfx, backgroundColor);
	quadMesh.vertexBuffer.Bind(gfx);
	quadMesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(quadMesh.indexBuffer.GetCount());

	if (clampedRatio <= 0.0f)
	{
		return;
	}

	const float fillWidth = width * clampedRatio;
	const float left = bar.center.x - width * 0.5f;

	XMStoreFloat4x4(
		&transformData.transform,
		XMMatrixTranspose(XMMatrixScaling(fillWidth, height, 1.0f) * XMMatrixTranslation(left + fillWidth * 0.5f, bar.center.y, 0.0f)));

	const ColorData fillColor = { bar.fillTint, 0.0f };
	transformBuffer.Update(gfx, transformData);
	colorBuffer.Update(gfx, fillColor);
	quadMesh.vertexBuffer.Bind(gfx);
	quadMesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(quadMesh.indexBuffer.GetCount());
}

void UIRenderer::RenderHealthBar(const HealthBar& bar) noexcept
{
	BindSharedState();
	DrawQuad(bar, bar.Ratio());
}

void UIRenderer::Render(const HudState& hudState) noexcept
{
	BindSharedState();

	const HealthBar playerBar
	{
		hudState.playerHealth.current,
		hudState.playerHealth.max,
		{ -0.62f, 0.88f },
		{ 0.48f, 0.06f },
		{ 0.18f, 0.84f, 0.36f },
		{ 0.08f, 0.09f, 0.12f },
		true
	};
	DrawQuad(playerBar, playerBar.Ratio());

	const HealthBar cooldownBar
	{
		1.0f - hudState.attackCooldown01,
		1.0f,
		{ -0.62f, 0.78f },
		{ 0.48f, 0.03f },
		{ 0.95f, 0.78f, 0.22f },
		{ 0.08f, 0.09f, 0.12f },
		true
	};
	DrawQuad(cooldownBar, cooldownBar.Ratio());

	if (hudState.hasTarget)
	{
		const HealthBar targetBar
		{
			hudState.targetHealth.current,
			hudState.targetHealth.max,
			{ 0.00f, 0.88f },
			{ 0.42f, 0.06f },
			{ 0.92f, 0.24f, 0.22f },
			{ 0.08f, 0.09f, 0.12f },
			true
		};
		DrawQuad(targetBar, targetBar.Ratio());
	}
}
