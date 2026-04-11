#include "Renderer.h"
#include "../Engine/Render/ImageLoader.h"
#include <DirectXMath.h>

Renderer::MeshBuffers::MeshBuffers(Graphics& gfx, const LitVertex* vertices, UINT vertexCount, const unsigned short* indices, UINT indexCount)
	:
	vertexBuffer(gfx, vertices, vertexCount * static_cast<UINT>(sizeof(LitVertex)), sizeof(LitVertex)),
	indexBuffer(gfx, indices, indexCount)
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
	litVertexShader(gfx, LitPipeline::GetShaderSource(), "VSMain"),
	litPixelShader(gfx, LitPipeline::GetShaderSource(), "PSMain"),
	litInputLayout(gfx, LitPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(LitPipeline::GetInputLayoutDesc().size()), litVertexShader),
	transformBuffer(gfx),
	materialBuffer(gfx),
	defaultWhiteTexture(gfx, MakeWhiteSurface()),
	samplerWrapLinear(gfx),
	samplerClampLinear(gfx, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP),
	samplerWrapPoint(gfx, D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP),
	topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{
}

const std::array<Renderer::LitVertex, 24>& Renderer::GetCubeVertices() noexcept
{
	static const std::array<LitVertex, 24> vertices =
	{
		// Back (-Z), normal (0,0,-1)
		LitVertex{ -1,-1,-1,  0, 0,-1,  0,1 },
		LitVertex{  1,-1,-1,  0, 0,-1,  1,1 },
		LitVertex{ -1, 1,-1,  0, 0,-1,  0,0 },
		LitVertex{  1, 1,-1,  0, 0,-1,  1,0 },
		// Front (+Z), normal (0,0,+1)
		LitVertex{ -1,-1, 1,  0, 0, 1,  0,1 },
		LitVertex{  1,-1, 1,  0, 0, 1,  1,1 },
		LitVertex{ -1, 1, 1,  0, 0, 1,  0,0 },
		LitVertex{  1, 1, 1,  0, 0, 1,  1,0 },
		// Right (+X), normal (1,0,0)
		LitVertex{  1,-1,-1,  1, 0, 0,  0,1 },
		LitVertex{  1,-1, 1,  1, 0, 0,  1,1 },
		LitVertex{  1, 1,-1,  1, 0, 0,  0,0 },
		LitVertex{  1, 1, 1,  1, 0, 0,  1,0 },
		// Left (-X), normal (-1,0,0)
		LitVertex{ -1,-1, 1, -1, 0, 0,  0,1 },
		LitVertex{ -1,-1,-1, -1, 0, 0,  1,1 },
		LitVertex{ -1, 1, 1, -1, 0, 0,  0,0 },
		LitVertex{ -1, 1,-1, -1, 0, 0,  1,0 },
		// Top (+Y), normal (0,1,0)
		LitVertex{ -1, 1,-1,  0, 1, 0,  0,1 },
		LitVertex{  1, 1,-1,  0, 1, 0,  1,1 },
		LitVertex{ -1, 1, 1,  0, 1, 0,  0,0 },
		LitVertex{  1, 1, 1,  0, 1, 0,  1,0 },
		// Bottom (-Y), normal (0,-1,0)
		LitVertex{ -1,-1,-1,  0,-1, 0,  0,1 },
		LitVertex{  1,-1,-1,  0,-1, 0,  1,1 },
		LitVertex{ -1,-1, 1,  0,-1, 0,  0,0 },
		LitVertex{  1,-1, 1,  0,-1, 0,  1,0 },
	};
	return vertices;
}

const std::array<unsigned short, 36>& Renderer::GetCubeIndices() noexcept
{
	static const std::array<unsigned short, 36> indices =
	{
		// Back (-Z): winding 0,2,1  2,3,1
		 0, 2, 1,   2, 3, 1,
		// Front (+Z): winding 0,1,2  1,3,2
		 4, 5, 6,   5, 7, 6,
		// Right (+X): winding 0,2,1  2,3,1
		 8,10, 9,  10,11, 9,
		// Left (-X): winding 0,2,1  2,3,1
		12,14,13,  14,15,13,
		// Top (+Y): winding 0,2,1  2,3,1
		16,18,17,  18,19,17,
		// Bottom (-Y): winding 0,1,2  1,3,2
		20,21,22,  21,23,22,
	};
	return indices;
}

const std::array<Renderer::LitVertex, 4>& Renderer::GetPlaneVertices() noexcept
{
	static const std::array<LitVertex, 4> vertices =
	{
		LitVertex{ -1,0,-1,  0,1,0,  0,9 },
		LitVertex{  1,0,-1,  0,1,0,  9,9 },
		LitVertex{ -1,0, 1,  0,1,0,  0,0 },
		LitVertex{  1,0, 1,  0,1,0,  9,0 },
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

void Renderer::BindLitState() noexcept
{
	litInputLayout.Bind(gfx);
	topology.Bind(gfx);
	litVertexShader.Bind(gfx);
	transformBuffer.Bind(gfx);
	materialBuffer.Bind(gfx, 1u);
	litPixelShader.Bind(gfx);
}

const Renderer::MeshBuffers& Renderer::ResolveMesh(RenderMeshType meshType) const noexcept
{
	switch (meshType)
	{
	case RenderMeshType::Plane: return planeMesh;
	case RenderMeshType::Cube:
	default:                    return cubeMesh;
	}
}

void Renderer::DrawMesh(const MeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept
{
	using namespace DirectX;

	const XMMATRIX model = transform.GetMatrix();
	const XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	TransformData transformData = {};
	XMStoreFloat4x4(&transformData.transform, XMMatrixTranspose(model * viewProjection));
	transformBuffer.Update(gfx, transformData);

	const Material mat = renderComponent.material.value_or(Material{});
	const MaterialData materialData = { mat.tint, 0.0f };
	materialBuffer.Update(gfx, materialData);

	ResolveTexture(mat.texturePath).Bind(gfx);
	ResolveSampler(mat.sampler).Bind(gfx);

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
	BindLitState();
	for (const auto& object : scene.GetObjects())
	{
		if (!object.CanRender())
		{
			continue;
		}
		DrawMesh(ResolveMesh(object.renderComponent->meshType), object.transform, *object.renderComponent, camera);
	}
}
