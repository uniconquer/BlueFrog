#include "Renderer.h"
#include "../Engine/Render/ImageLoader.h"
#include <DirectXMath.h>

Renderer::MeshBuffers::MeshBuffers(Graphics& gfx, const Vertex* vertices, UINT vertexCount, const unsigned short* indices, UINT indexCount)
	:
	vertexBuffer(gfx, vertices, vertexCount * static_cast<UINT>(sizeof(Vertex)), sizeof(Vertex)),
	indexBuffer(gfx, indices, indexCount)
{
}

Renderer::TexturedMeshBuffers::TexturedMeshBuffers(Graphics& gfx)
	:
	vertexBuffer(gfx, GetTexturedPlaneVertices().data(), static_cast<UINT>(GetTexturedPlaneVertices().size()) * static_cast<UINT>(sizeof(TexturedVertex)), sizeof(TexturedVertex)),
	indexBuffer(gfx, GetTexturedPlaneIndices().data(), static_cast<UINT>(GetTexturedPlaneIndices().size()))
{
}

static Surface MakeWhiteSurface()
{
	Surface s(1u, 1u);
	std::uint8_t* const p = s.GetPixels();
	p[0] = p[1] = p[2] = p[3] = 255u;
	return s;
}

Renderer::Renderer(Graphics& gfx)
	:
	gfx(gfx),
	cubeMesh(gfx, GetCubeVertices().data(), static_cast<UINT>(GetCubeVertices().size()), GetCubeIndices().data(), static_cast<UINT>(GetCubeIndices().size())),
	planeMesh(gfx, GetPlaneVertices().data(), static_cast<UINT>(GetPlaneVertices().size()), GetPlaneIndices().data(), static_cast<UINT>(GetPlaneIndices().size())),
	texturedPlaneMesh(gfx),
	vertexShader(gfx, FlatColorPipeline::GetShaderSource(), "VSMain"),
	pixelShader(gfx, FlatColorPipeline::GetShaderSource(), "PSMain"),
	inputLayout(gfx, FlatColorPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(FlatColorPipeline::GetInputLayoutDesc().size()), vertexShader),
	texturedVertexShader(gfx, TexturedPipeline::GetShaderSource(), "VSMain"),
	texturedPixelShader(gfx, TexturedPipeline::GetShaderSource(), "PSMain"),
	texturedInputLayout(gfx, TexturedPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(TexturedPipeline::GetInputLayoutDesc().size()), texturedVertexShader),
	transformBuffer(gfx),
	colorBuffer(gfx),
	defaultWhiteTexture(gfx, MakeWhiteSurface()),
	samplerWrapLinear(gfx),
	samplerClampLinear(gfx, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP),
	samplerWrapPoint(gfx, D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP),
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

const std::array<Renderer::TexturedVertex, 4>& Renderer::GetTexturedPlaneVertices() noexcept
{
	static const std::array<TexturedVertex, 4> vertices =
	{
		TexturedVertex{ -1.0f, 0.0f, -1.0f, 0.0f, 9.0f },
		TexturedVertex{ 1.0f, 0.0f, -1.0f, 9.0f, 9.0f },
		TexturedVertex{ -1.0f, 0.0f, 1.0f, 0.0f, 0.0f },
		TexturedVertex{ 1.0f, 0.0f, 1.0f, 9.0f, 0.0f },
	};
	return vertices;
}

const std::array<unsigned short, 6>& Renderer::GetTexturedPlaneIndices() noexcept
{
	static const std::array<unsigned short, 6> indices =
	{
		0, 2, 1,
		2, 3, 1,
	};
	return indices;
}

void Renderer::BindFlatState() noexcept
{
	inputLayout.Bind(gfx);
	topology.Bind(gfx);
	vertexShader.Bind(gfx);
	transformBuffer.Bind(gfx);
	colorBuffer.Bind(gfx);
	pixelShader.Bind(gfx);
}

void Renderer::BindTexturedState() noexcept
{
	texturedInputLayout.Bind(gfx);
	topology.Bind(gfx);
	texturedVertexShader.Bind(gfx);
	transformBuffer.Bind(gfx);
	colorBuffer.Bind(gfx);
	texturedPixelShader.Bind(gfx);
	// Texture and sampler are bound per-draw in DrawTexturedMesh
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

const Renderer::TexturedMeshBuffers& Renderer::ResolveTexturedMesh(RenderMeshType meshType) const noexcept
{
	switch (meshType)
	{
	case RenderMeshType::Plane:
	default:
		return texturedPlaneMesh;
	}
}

void Renderer::DrawFlatMesh(const MeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept
{
	using namespace DirectX;

	BindFlatState();

	const XMMATRIX model = transform.GetMatrix();
	const XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	TransformData transformData = {};
	XMStoreFloat4x4(&transformData.transform, XMMatrixTranspose(model * viewProjection));

	const ColorData colorData = { renderComponent.tint, 0.0f };
	transformBuffer.Update(gfx, transformData);
	colorBuffer.Update(gfx, colorData);
	mesh.vertexBuffer.Bind(gfx);
	mesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(mesh.indexBuffer.GetCount());
}

void Renderer::DrawTexturedMesh(const TexturedMeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept
{
	using namespace DirectX;

	BindTexturedState();

	const XMMATRIX model = transform.GetMatrix();
	const XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	TransformData transformData = {};
	XMStoreFloat4x4(&transformData.transform, XMMatrixTranspose(model * viewProjection));
	transformBuffer.Update(gfx, transformData);

	XMFLOAT3 tint = renderComponent.tint;
	if (renderComponent.material.has_value())
	{
		tint = renderComponent.material->tint;
		ResolveTexture(renderComponent.material->texturePath).Bind(gfx);
		ResolveSampler(renderComponent.material->sampler).Bind(gfx);
	}
	else
	{
		defaultWhiteTexture.Bind(gfx);
		samplerWrapLinear.Bind(gfx);
	}

	const ColorData colorData = { tint, 0.0f };
	colorBuffer.Update(gfx, colorData);
	mesh.vertexBuffer.Bind(gfx);
	mesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(mesh.indexBuffer.GetCount());
}

Texture2D& Renderer::ResolveTexture(const std::string& path)
{
	if (path.empty())
	{
		return defaultWhiteTexture;
	}
	auto it = textureCache.find(path);
	if (it != textureCache.end())
	{
		return it->second;
	}
	Surface surface = ImageLoader::LoadSurfaceFromFile(std::wstring(path.begin(), path.end()));
	auto [inserted_it, ok] = textureCache.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(path),
		std::forward_as_tuple(gfx, surface));
	return inserted_it->second;
}

const Sampler& Renderer::ResolveSampler(SamplerPreset preset) const noexcept
{
	switch (preset)
	{
	case SamplerPreset::ClampLinear: return samplerClampLinear;
	case SamplerPreset::WrapPoint:   return samplerWrapPoint;
	case SamplerPreset::WrapLinear:
	default:                          return samplerWrapLinear;
	}
}

void Renderer::Render(const Scene& scene, const TopDownCamera& camera) noexcept
{
	for (const auto& object : scene.GetObjects())
	{
		if (!object.CanRender())
		{
			continue;
		}

		const auto& renderComponent = *object.renderComponent;
		if (renderComponent.visualKind == RenderVisualKind::Textured && renderComponent.meshType == RenderMeshType::Plane)
		{
			DrawTexturedMesh(ResolveTexturedMesh(renderComponent.meshType), object.transform, renderComponent, camera);
		}
		else
		{
			DrawFlatMesh(ResolveMesh(renderComponent.meshType), object.transform, renderComponent, camera);
		}
	}
}
