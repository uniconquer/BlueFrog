#include "Renderer.h"
#include <DirectXMath.h>

Renderer::Renderer(Graphics& gfx)
	:
	gfx(gfx),
	vertexBuffer(gfx, GetCubeVertices().data(), static_cast<UINT>(sizeof(Vertex) * GetCubeVertices().size()), sizeof(Vertex)),
	indexBuffer(gfx, GetCubeIndices().data(), static_cast<UINT>(GetCubeIndices().size())),
	vertexShader(gfx, GetSolidShaderSource(), "VSMain"),
	pixelShader(gfx, GetSolidShaderSource(), "PSMain"),
	inputLayout(gfx, GetInputLayoutDesc().data(), static_cast<UINT>(GetInputLayoutDesc().size()), vertexShader),
	transformBuffer(gfx),
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
		"    return float4(input.color, 1.0f);\n"
		"}\n";
}

void Renderer::DrawTestCube(float angle) noexcept
{
	using namespace DirectX;

	const XMMATRIX model = XMMatrixRotationRollPitchYaw(angle * 0.45f, angle, angle * 0.2f);
	const XMMATRIX view = XMMatrixLookAtLH(
		XMVectorSet(0.0f, 2.2f, -6.0f, 1.0f),
		XMVectorZero(),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	const XMMATRIX projection = XMMatrixPerspectiveFovLH(1.0f, 800.0f / 600.0f, 0.5f, 40.0f);

	TransformData transform = {};
	XMStoreFloat4x4(&transform.transform, XMMatrixTranspose(model * view * projection));

	transformBuffer.Update(gfx, transform);
	inputLayout.Bind(gfx);
	vertexBuffer.Bind(gfx);
	indexBuffer.Bind(gfx);
	topology.Bind(gfx);
	vertexShader.Bind(gfx);
	transformBuffer.Bind(gfx);
	pixelShader.Bind(gfx);
	gfx.DrawIndexed(indexBuffer.GetCount());
}
