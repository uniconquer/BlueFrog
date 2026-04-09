#include "Renderer.h"
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

void Renderer::BindSharedState() noexcept
{
	inputLayout.Bind(gfx);
	topology.Bind(gfx);
	vertexShader.Bind(gfx);
	transformBuffer.Bind(gfx);
	pixelShader.Bind(gfx);
}

void Renderer::DrawMesh(const MeshBuffers& mesh, const Transform& transform, const TopDownCamera& camera) noexcept
{
	using namespace DirectX;

	const XMMATRIX model = transform.GetMatrix();
	const XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	TransformData transformData = {};
	XMStoreFloat4x4(&transformData.transform, XMMatrixTranspose(model * viewProjection));

	transformBuffer.Update(gfx, transformData);
	mesh.vertexBuffer.Bind(gfx);
	mesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(mesh.indexBuffer.GetCount());
}

void Renderer::DrawTestScene(const TopDownCamera& camera, float time) noexcept
{
	BindSharedState();

	Transform ground = {};
	ground.scale = { 18.0f, 1.0f, 18.0f };
	DrawMesh(planeMesh, ground, camera);

	Transform central = {};
	central.position = { 0.0f, 1.25f, 0.0f };
	central.scale = { 1.35f, 1.35f, 1.35f };
	central.rotation = { time * 0.15f, time, time * 0.08f };
	DrawMesh(cubeMesh, central, camera);

	Transform northWall = {};
	northWall.position = { 0.0f, 1.0f, 6.5f };
	northWall.scale = { 6.0f, 1.0f, 0.7f };
	DrawMesh(cubeMesh, northWall, camera);

	Transform southWall = {};
	southWall.position = { 0.0f, 1.0f, -6.5f };
	southWall.scale = { 6.0f, 1.0f, 0.7f };
	DrawMesh(cubeMesh, southWall, camera);

	Transform eastWall = {};
	eastWall.position = { 6.5f, 1.0f, 0.0f };
	eastWall.scale = { 0.7f, 1.0f, 6.0f };
	DrawMesh(cubeMesh, eastWall, camera);

	Transform westWall = {};
	westWall.position = { -6.5f, 1.0f, 0.0f };
	westWall.scale = { 0.7f, 1.0f, 6.0f };
	DrawMesh(cubeMesh, westWall, camera);

	Transform pillarA = {};
	pillarA.position = { -3.5f, 0.8f, -2.5f };
	pillarA.scale = { 0.8f, 0.8f, 0.8f };
	pillarA.rotation = { 0.0f, time * 0.5f, 0.0f };
	DrawMesh(cubeMesh, pillarA, camera);

	Transform pillarB = {};
	pillarB.position = { 3.5f, 0.8f, 2.5f };
	pillarB.scale = { 0.8f, 0.8f, 0.8f };
	pillarB.rotation = { 0.0f, -time * 0.45f, 0.0f };
	DrawMesh(cubeMesh, pillarB, camera);
}
