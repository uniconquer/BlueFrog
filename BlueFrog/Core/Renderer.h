#pragma once
#include "Graphics.h"
#include "../Engine/Camera/TopDownCamera.h"
#include "../Engine/Render/ConstantBuffer.h"
#include "../Engine/Render/IndexBuffer.h"
#include "../Engine/Render/InputLayout.h"
#include "../Engine/Render/LitPipeline.h"
#include "../Engine/Render/PixelShader.h"
#include "../Engine/Render/Sampler.h"
#include "../Engine/Render/Texture2D.h"
#include "../Engine/Render/Topology.h"
#include "../Engine/Render/VertexBuffer.h"
#include "../Engine/Render/VertexShader.h"
#include "../Engine/Scene/Material.h"
#include "../Engine/Scene/RenderComponent.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/Scene/Transform.h"
#include <DirectXMath.h>
#include <array>
#include <string>
#include <unordered_map>

class Renderer
{
private:
	struct LitVertex
	{
		float x, y, z;
		float nx, ny, nz;
		float u, v;
	};

	struct TransformData
	{
		DirectX::XMFLOAT4X4 mvp;
		DirectX::XMFLOAT4X4 model;
	};

	struct MaterialData
	{
		DirectX::XMFLOAT3 tint;
		float pad0 = 0.0f;
	};

	struct LightData
	{
		DirectX::XMFLOAT3 lightDir;
		float ambient = 0.0f;
		DirectX::XMFLOAT3 lightColor;
		float pad1 = 0.0f;
	};

	struct MeshBuffers
	{
		MeshBuffers(Graphics& gfx, const LitVertex* vertices, UINT vertexCount, const unsigned short* indices, UINT indexCount);

		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
	};

public:
	explicit Renderer(Graphics& gfx);
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	void Render(const Scene& scene, const TopDownCamera& camera) noexcept;

private:
	void BindLitState() noexcept;
	const MeshBuffers& ResolveMesh(const RenderComponent& renderComponent);
	void DrawMesh(const MeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept;
	Texture2D& ResolveTexture(const std::string& path);
	const Sampler& ResolveSampler(SamplerPreset preset) const noexcept;
	static const std::array<LitVertex, 24>& GetCubeVertices() noexcept;
	static const std::array<unsigned short, 36>& GetCubeIndices() noexcept;
	static const std::array<LitVertex, 4>& GetPlaneVertices() noexcept;
	static const std::array<unsigned short, 6>& GetPlaneIndices() noexcept;

private:
	Graphics& gfx;
	MeshBuffers cubeMesh;
	MeshBuffers planeMesh;
	// Imported glTF meshes, keyed by source path. Loaded lazily on the first
	// frame a scene object referencing the path is rendered. Same lifetime
	// model as textureCache below — survives scene reloads.
	std::unordered_map<std::string, MeshBuffers> importedMeshCache;
	VertexShader litVertexShader;
	PixelShader litPixelShader;
	InputLayout litInputLayout;
	VertexConstantBuffer<TransformData> transformBuffer;
	PixelConstantBuffer<MaterialData> materialBuffer;
	PixelConstantBuffer<LightData> lightBuffer;
	Texture2D defaultWhiteTexture;
	std::unordered_map<std::string, Texture2D> textureCache;
	Sampler samplerWrapLinear;
	Sampler samplerClampLinear;
	Sampler samplerWrapPoint;
	Topology topology;
};
