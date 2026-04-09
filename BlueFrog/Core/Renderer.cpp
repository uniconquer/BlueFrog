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
	vertexShader(gfx, FlatColorPipeline::GetShaderSource(), "VSMain"),
	pixelShader(gfx, FlatColorPipeline::GetShaderSource(), "PSMain"),
	inputLayout(gfx, FlatColorPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(FlatColorPipeline::GetInputLayoutDesc().size()), vertexShader),
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

void Renderer::Render(const Scene& scene, const TopDownCamera& camera) noexcept
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
}
