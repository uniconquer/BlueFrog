#include "Renderer.h"
#include <algorithm>
#include <DirectXMath.h>

Renderer::MeshBuffers::MeshBuffers(Graphics& gfx, const Vertex* vertices, UINT vertexCount, const unsigned short* indices, UINT indexCount)
	:
	vertexBuffer(gfx, vertices, vertexCount * static_cast<UINT>(sizeof(Vertex)), sizeof(Vertex)),
	indexBuffer(gfx, indices, indexCount)
{
}

Renderer::Renderer(Graphics& gfx)
	:
	gfx(gfx),
	cubeMesh(gfx, GetCubeVertices().data(), static_cast<UINT>(GetCubeVertices().size()), GetCubeIndices().data(), static_cast<UINT>(GetCubeIndices().size())),
	planeMesh(gfx, GetPlaneVertices().data(), static_cast<UINT>(GetPlaneVertices().size()), GetPlaneIndices().data(), static_cast<UINT>(GetPlaneIndices().size())),
	hudQuadMesh(gfx, GetHudQuadVertices().data(), static_cast<UINT>(GetHudQuadVertices().size()), GetHudQuadIndices().data(), static_cast<UINT>(GetHudQuadIndices().size())),
	vertexShader(gfx, GetSolidShaderSource(), "VSMain"),
	pixelShader(gfx, GetSolidShaderSource(), "PSMain"),
	inputLayout(gfx, GetInputLayoutDesc().data(), static_cast<UINT>(GetInputLayoutDesc().size()), vertexShader),
	transformBuffer(gfx),
	colorBuffer(gfx),
	topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{
}

const std::array<Renderer::Vertex, 8>& Renderer::GetCubeVertices() noexcept
{
	static const std::array<Vertex, 8> vertices =
	{
		Vertex{ -1.0f, -1.0f, -1.0f, 0.95f, 0.35f, 0.20f },
		Vertex{ 1.0f, -1.0f, -1.0f, 0.15f, 0.85f, 0.45f },
		Vertex{ -1.0f, 1.0f, -1.0f, 0.20f, 0.50f, 0.95f },
		Vertex{ 1.0f, 1.0f, -1.0f, 0.95f, 0.90f, 0.25f },
		Vertex{ -1.0f, -1.0f, 1.0f, 0.85f, 0.25f, 0.80f },
		Vertex{ 1.0f, -1.0f, 1.0f, 0.20f, 0.85f, 0.90f },
		Vertex{ -1.0f, 1.0f, 1.0f, 0.95f, 0.55f, 0.35f },
		Vertex{ 1.0f, 1.0f, 1.0f, 0.80f, 0.95f, 0.35f },
	};
	return vertices;
}

const std::array<unsigned short, 36>& Renderer::GetCubeIndices() noexcept
{
	static const std::array<unsigned short, 36> indices =
	{
		0, 2, 1, 2, 3, 1,
		1, 3, 5, 3, 7, 5,
		2, 6, 3, 3, 6, 7,
		4, 5, 7, 4, 7, 6,
		0, 4, 2, 2, 4, 6,
		0, 1, 4, 1, 5, 4,
	};
	return indices;
}

const std::array<Renderer::Vertex, 4>& Renderer::GetPlaneVertices() noexcept
{
	static const std::array<Vertex, 4> vertices =
	{
		Vertex{ -1.0f, 0.0f, -1.0f, 0.18f, 0.48f, 0.20f },
		Vertex{ 1.0f, 0.0f, -1.0f, 0.20f, 0.55f, 0.22f },
		Vertex{ -1.0f, 0.0f, 1.0f, 0.17f, 0.45f, 0.18f },
		Vertex{ 1.0f, 0.0f, 1.0f, 0.22f, 0.58f, 0.24f },
	};
	return vertices;
}

const std::array<unsigned short, 6>& Renderer::GetPlaneIndices() noexcept
{
	static const std::array<unsigned short, 6> indices =
	{
		0, 2, 1,
		2, 3, 1,
	};
	return indices;
}

const std::array<Renderer::Vertex, 4>& Renderer::GetHudQuadVertices() noexcept
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

const std::array<unsigned short, 6>& Renderer::GetHudQuadIndices() noexcept
{
	static const std::array<unsigned short, 6> indices =
	{
		0, 2, 1,
		2, 3, 1,
	};
	return indices;
}

const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& Renderer::GetInputLayoutDesc() noexcept
{
	static const std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputLayoutDesc =
	{
		D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
		D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
	};
	return inputLayoutDesc;
}

const char* Renderer::GetSolidShaderSource() noexcept
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

void Renderer::BindSharedState() noexcept
{
	inputLayout.Bind(gfx);
	topology.Bind(gfx);
	vertexShader.Bind(gfx);
	transformBuffer.Bind(gfx);
	colorBuffer.Bind(gfx);
	pixelShader.Bind(gfx);
}

const Renderer::MeshBuffers& Renderer::ResolveMesh(RenderMeshType meshType) const noexcept
{
	switch (meshType)
	{
	case RenderMeshType::Plane:
		return planeMesh;
	case RenderMeshType::Cube:
	default:
		return cubeMesh;
	}
}

void Renderer::DrawMesh(const MeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept
{
	using namespace DirectX;

	const XMMATRIX model = transform.GetMatrix();
	const XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	TransformData transformData = {};
	XMStoreFloat4x4(&transformData.transform, XMMatrixTranspose(model * viewProjection));

	const ColorData colorData = { renderComponent.tint,0.0f };
	transformBuffer.Update(gfx, transformData);
	colorBuffer.Update(gfx, colorData);
	mesh.vertexBuffer.Bind(gfx);
	mesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(mesh.indexBuffer.GetCount());
}

void Renderer::DrawHudQuad(float centerX, float centerY, float width, float height, const DirectX::XMFLOAT3& tint) noexcept
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

	const ColorData colorData = { tint,0.0f };
	transformBuffer.Update(gfx, transformData);
	colorBuffer.Update(gfx, colorData);
	hudQuadMesh.vertexBuffer.Bind(gfx);
	hudQuadMesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(hudQuadMesh.indexBuffer.GetCount());
}

void Renderer::RenderHud(const HudState& hudState) noexcept
{
	const auto drawMeter = [this](float centerX, float centerY, float width, float height, float ratio, const DirectX::XMFLOAT3& fillTint)
	{
		constexpr DirectX::XMFLOAT3 backgroundTint = { 0.08f, 0.09f, 0.12f };
		DrawHudQuad(centerX, centerY, width, height, backgroundTint);

		const float clampedRatio = std::clamp(ratio, 0.0f, 1.0f);
		if (clampedRatio <= 0.0f)
		{
			return;
		}

		const float fillWidth = width * clampedRatio;
		const float left = centerX - width * 0.5f;
		DrawHudQuad(left + fillWidth * 0.5f, centerY, fillWidth, height, fillTint);
	};

	drawMeter(-0.62f, 0.88f, 0.48f, 0.06f, hudState.playerHealth.Ratio(), { 0.18f, 0.84f, 0.36f });
	drawMeter(-0.62f, 0.78f, 0.48f, 0.03f, hudState.attackCooldown01, { 0.95f, 0.78f, 0.22f });

	if (hudState.hasTarget)
	{
		drawMeter(0.00f, 0.88f, 0.42f, 0.06f, hudState.targetHealth.Ratio(), { 0.92f, 0.24f, 0.22f });
	}
}

void Renderer::Render(const Scene& scene, const TopDownCamera& camera, const HudState& hudState) noexcept
{
	BindSharedState();

	for (const auto& object : scene.GetObjects())
	{
		if (!object.CanRender())
		{
			continue;
		}

		const auto& renderComponent = *object.renderComponent;
		DrawMesh(ResolveMesh(renderComponent.meshType), object.transform, renderComponent, camera);
	}

	RenderHud(hudState);
}
