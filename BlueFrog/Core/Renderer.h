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
	void DrawTestScene(const TopDownCamera& camera, float time) noexcept;
private:
	void BindSharedState() noexcept;
	void DrawMesh(const MeshBuffers& mesh, const Transform& transform, const TopDownCamera& camera) noexcept;
	static const std::array<Vertex, 8>& GetCubeVertices() noexcept;
	static const std::array<unsigned short, 36>& GetCubeIndices() noexcept;
	static const std::array<Vertex, 4>& GetPlaneVertices() noexcept;
	static const std::array<unsigned short, 6>& GetPlaneIndices() noexcept;
	static const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& GetInputLayoutDesc() noexcept;
	static const char* GetSolidShaderSource() noexcept;
private:
	Graphics& gfx;
	MeshBuffers cubeMesh;
	MeshBuffers planeMesh;
	VertexShader vertexShader;
	PixelShader pixelShader;
	InputLayout inputLayout;
	VertexConstantBuffer<TransformData> transformBuffer;
	Topology topology;
};
