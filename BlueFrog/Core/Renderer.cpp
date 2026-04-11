#include "Renderer.h"
#include <algorithm>
#include <cstdint>
#include <vector>
#include <DirectXMath.h>

namespace
{
	struct Rgba8
	{
		std::uint8_t r;
		std::uint8_t g;
		std::uint8_t b;
		std::uint8_t a;
	};

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCheckerboardTexture(Graphics& gfx)
	{
		constexpr UINT width = 64u;
		constexpr UINT height = 64u;
		constexpr UINT tileSize = 8u;

		std::vector<Rgba8> pixels(width * height);
		for (UINT y = 0u; y < height; ++y)
		{
			for (UINT x = 0u; x < width; ++x)
			{
				const bool checker = ((x / tileSize) + (y / tileSize)) % 2u == 0u;
				pixels[y * width + x] = checker
					? Rgba8{ 95u, 118u, 74u, 255u }
					: Rgba8{ 123u, 97u, 62u, 255u };
			}
		}

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1u;
		desc.ArraySize = 1u;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1u;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = pixels.data();
		initData.SysMemPitch = width * static_cast<UINT>(sizeof(Rgba8));

		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
		if (const HRESULT hr = gfx.GetDevice()->CreateTexture2D(&desc, &initData, texture.GetAddressOf()); FAILED(hr))
		{
			throw BFGFX_EXCEPT(hr);
		}

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureView;
		if (const HRESULT hr = gfx.GetDevice()->CreateShaderResourceView(texture.Get(), nullptr, textureView.GetAddressOf()); FAILED(hr))
		{
			throw BFGFX_EXCEPT(hr);
		}

		return textureView;
	}

	Microsoft::WRL::ComPtr<ID3D11SamplerState> CreateWrapSampler(Graphics& gfx)
	{
		D3D11_SAMPLER_DESC desc{};
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MinLOD = 0.0f;
		desc.MaxLOD = D3D11_FLOAT32_MAX;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
		if (const HRESULT hr = gfx.GetDevice()->CreateSamplerState(&desc, sampler.GetAddressOf()); FAILED(hr))
		{
			throw BFGFX_EXCEPT(hr);
		}

		return sampler;
	}

	namespace TexturedPipeline
	{
		inline const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& GetInputLayoutDesc() noexcept
		{
			static const std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputLayoutDesc =
			{
				D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
				D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
			};
			return inputLayoutDesc;
		}

		inline const char* GetShaderSource() noexcept
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
				"Texture2D groundTexture : register(t0);\n"
				"SamplerState groundSampler : register(s0);\n"
				"struct VSIn\n"
				"{\n"
				"    float3 pos : POSITION;\n"
				"    float2 uv : TEXCOORD;\n"
				"};\n"
				"struct PSIn\n"
				"{\n"
				"    float4 pos : SV_Position;\n"
				"    float2 uv : TEXCOORD;\n"
				"};\n"
				"PSIn VSMain(VSIn input)\n"
				"{\n"
				"    PSIn output;\n"
				"    output.pos = mul(float4(input.pos, 1.0f), transform);\n"
				"    output.uv = input.uv;\n"
				"    return output;\n"
				"}\n"
				"float4 PSMain(PSIn input) : SV_Target\n"
				"{\n"
				"    return groundTexture.Sample(groundSampler, input.uv) * float4(tint, 1.0f);\n"
				"}\n";
		}
	}
}

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
	groundTextureView(CreateCheckerboardTexture(gfx)),
	groundSampler(CreateWrapSampler(gfx)),
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

	ID3D11ShaderResourceView* const textureView = groundTextureView.Get();
	ID3D11SamplerState* const sampler = groundSampler.Get();
	gfx.GetContext()->PSSetShaderResources(0u, 1u, &textureView);
	gfx.GetContext()->PSSetSamplers(0u, 1u, &sampler);
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

	const ColorData colorData = { renderComponent.tint, 0.0f };
	transformBuffer.Update(gfx, transformData);
	colorBuffer.Update(gfx, colorData);
	mesh.vertexBuffer.Bind(gfx);
	mesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(mesh.indexBuffer.GetCount());
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
