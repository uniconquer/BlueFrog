#include "UIRenderer.h"
#include <algorithm>

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
	vertexShader(gfx, FlatColorPipeline::GetShaderSource(), "VSMain"),
	pixelShader(gfx, FlatColorPipeline::GetShaderSource(), "PSMain"),
	inputLayout(gfx, FlatColorPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(FlatColorPipeline::GetInputLayoutDesc().size()), vertexShader),
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

void UIRenderer::BindSharedState() noexcept
{
	inputLayout.Bind(gfx);
	topology.Bind(gfx);
	vertexShader.Bind(gfx);
	transformBuffer.Bind(gfx);
	colorBuffer.Bind(gfx);
	pixelShader.Bind(gfx);
}

void UIRenderer::DrawQuad(float centerX, float centerY, float width, float height, const DirectX::XMFLOAT3& tint) noexcept
{
	using namespace DirectX;

	if (width <= 0.0f || height <= 0.0f)
	{
		return;
	}

	TransformData transformData = {};
	XMStoreFloat4x4(
		&transformData.transform,
		XMMatrixTranspose(XMMatrixScaling(width, height, 1.0f) * XMMatrixTranslation(centerX, centerY, 0.0f)));

	const ColorData colorData = { tint, 0.0f };
	transformBuffer.Update(gfx, transformData);
	colorBuffer.Update(gfx, colorData);
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

	DrawBar(HealthBar{ -0.62f, 0.88f, 0.48f, 0.06f, hudState.playerHealth.Ratio(), { 0.08f, 0.09f, 0.12f }, { 0.18f, 0.84f, 0.36f } });
	DrawBar(HealthBar{ -0.62f, 0.78f, 0.48f, 0.03f, hudState.attackCooldown01, { 0.08f, 0.09f, 0.12f }, { 0.95f, 0.78f, 0.22f } });

	if (hudState.hasTarget)
	{
		DrawBar(HealthBar{ 0.00f, 0.88f, 0.42f, 0.06f, hudState.targetHealth.Ratio(), { 0.08f, 0.09f, 0.12f }, { 0.92f, 0.24f, 0.22f } });
	}
}
