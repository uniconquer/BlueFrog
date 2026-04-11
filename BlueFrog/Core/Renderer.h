#pragma once
#include "Graphics.h"
#include "../Engine/Camera/TopDownCamera.h"
#include "../Engine/Render/ConstantBuffer.h"
#include "../Engine/Render/FlatColorPipeline.h"
#include "../Engine/Render/IndexBuffer.h"
#include "../Engine/Render/InputLayout.h"
#include "../Engine/Render/PixelShader.h"
#include "../Engine/Render/Topology.h"
#include "../Engine/Render/VertexBuffer.h"
#include "../Engine/Render/VertexShader.h"
#include "../Engine/Scene/RenderComponent.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/Scene/Transform.h"
#include <DirectXMath.h>
#include <array>

class Renderer
{
private:
	struct Vertex
	{
		float x;
		float y;
		float z;
		float r;
		float g;
		float b;
	};

	struct TransformData
	{
		DirectX::XMFLOAT4X4 transform;
	};

	struct ColorData
	{
		DirectX::XMFLOAT3 tint;
		float padding = 0.0f;
	};

	struct MeshBuffers
	{
		MeshBuffers(Graphics& gfx, const Vertex* vertices, UINT vertexCount, const unsigned short* indices, UINT indexCount);

		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
	};

	struct TexturedVertex
	{
		float x;
		float y;
		float z;
		float u;
		float v;
	};

	struct TexturedMeshBuffers
	{
		TexturedMeshBuffers(Graphics& gfx);

		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
	};
public:
	explicit Renderer(Graphics& gfx);
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	void Render(const Scene& scene, const TopDownCamera& camera) noexcept;
private:
	void BindFlatState() noexcept;
	void BindTexturedState() noexcept;
	const MeshBuffers& ResolveMesh(RenderMeshType meshType) const noexcept;
	const TexturedMeshBuffers& ResolveTexturedMesh(RenderMeshType meshType) const noexcept;
	void DrawFlatMesh(const MeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept;
	void DrawTexturedMesh(const TexturedMeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept;
	static const std::array<Vertex, 8>& GetCubeVertices() noexcept;
	static const std::array<unsigned short, 36>& GetCubeIndices() noexcept;
	static const std::array<Vertex, 4>& GetPlaneVertices() noexcept;
	static const std::array<unsigned short, 6>& GetPlaneIndices() noexcept;
	static const std::array<TexturedVertex, 4>& GetTexturedPlaneVertices() noexcept;
	static const std::array<unsigned short, 6>& GetTexturedPlaneIndices() noexcept;
private:
	Graphics& gfx;
	MeshBuffers cubeMesh;
	MeshBuffers planeMesh;
	TexturedMeshBuffers texturedPlaneMesh;
	VertexShader vertexShader;
	PixelShader pixelShader;
	InputLayout inputLayout;
	VertexShader texturedVertexShader;
	PixelShader texturedPixelShader;
	InputLayout texturedInputLayout;
	VertexConstantBuffer<TransformData> transformBuffer;
	PixelConstantBuffer<ColorData> colorBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> groundTextureView;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> groundSampler;
	Topology topology;
};
