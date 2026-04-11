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

void UIRenderer::DrawCrosshair() noexcept
{
	using namespace UiLayout;

	const float armLength = CrosshairLength * 0.5f - CrosshairGap * 0.5f;
	if (armLength <= 0.0f)
	{
		return;
	}

	const float armOffset = CrosshairGap * 0.5f + armLength * 0.5f;
	DrawQuad(CrosshairCenterX - armOffset, CrosshairCenterY, armLength, CrosshairThickness, CrosshairTint);
	DrawQuad(CrosshairCenterX + armOffset, CrosshairCenterY, armLength, CrosshairThickness, CrosshairTint);
	DrawQuad(CrosshairCenterX, CrosshairCenterY - armOffset, CrosshairThickness, armLength, CrosshairTint);
	DrawQuad(CrosshairCenterX, CrosshairCenterY + armOffset, CrosshairThickness, armLength, CrosshairTint);
}

void UIRenderer::Render(const HudState& hudState) noexcept
{
	BindSharedState();

	DrawBar(UiLayout::MakePlayerHealthBar(hudState.playerHealth.Ratio()));
	DrawBar(UiLayout::MakeAttackCooldownBar(hudState.attackCooldown01));

	if (hudState.hasTarget)
	{
		DrawBar(UiLayout::MakeTargetHealthBar(hudState.targetHealth.Ratio()));
	}

	DrawCrosshair();
}
