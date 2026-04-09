#pragma once
#include "Graphics.h"
#include "../Engine/Camera/TopDownCamera.h"
#include "../Engine/Render/ConstantBuffer.h"
#include "../Engine/Render/IndexBuffer.h"
#include "../Engine/Render/InputLayout.h"
#include "../Engine/Render/PixelShader.h"
#include "../Engine/Render/Topology.h"
#include "../Engine/Render/VertexBuffer.h"
#include "../Engine/Render/VertexShader.h"
#include "../Engine/Scene/RenderComponent.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/Scene/Transform.h"
#include "../Engine/UI/HudState.h"
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
public:
	explicit Renderer(Graphics& gfx);
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	void Render(const Scene& scene, const TopDownCamera& camera, const HudState& hudState) noexcept;
private:
	void BindSharedState() noexcept;
	const MeshBuffers& ResolveMesh(RenderMeshType meshType) const noexcept;
	void DrawMesh(const MeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept;
	void DrawHudQuad(float centerX, float centerY, float width, float height, const DirectX::XMFLOAT3& tint) noexcept;
	void RenderHud(const HudState& hudState) noexcept;
	static const std::array<Vertex, 8>& GetCubeVertices() noexcept;
	static const std::array<unsigned short, 36>& GetCubeIndices() noexcept;
	static const std::array<Vertex, 4>& GetPlaneVertices() noexcept;
	static const std::array<unsigned short, 6>& GetPlaneIndices() noexcept;
	static const std::array<Vertex, 4>& GetHudQuadVertices() noexcept;
	static const std::array<unsigned short, 6>& GetHudQuadIndices() noexcept;
	static const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& GetInputLayoutDesc() noexcept;
	static const char* GetSolidShaderSource() noexcept;
private:
	Graphics& gfx;
	MeshBuffers cubeMesh;
	MeshBuffers planeMesh;
	MeshBuffers hudQuadMesh;
	VertexShader vertexShader;
	PixelShader pixelShader;
	InputLayout inputLayout;
	VertexConstantBuffer<TransformData> transformBuffer;
	PixelConstantBuffer<ColorData> colorBuffer;
	Topology topology;
};
