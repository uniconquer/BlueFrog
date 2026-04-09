#pragma once
#include "Graphics.h"
#include "../Engine/Render/ConstantBuffer.h"
#include "../Engine/Render/IndexBuffer.h"
#include "../Engine/Render/InputLayout.h"
#include "../Engine/Render/PixelShader.h"
#include "../Engine/Render/Topology.h"
#include "../Engine/Render/VertexBuffer.h"
#include "../Engine/Render/VertexShader.h"
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
public:
	explicit Renderer(Graphics& gfx);
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	void DrawTestCube(float angle) noexcept;
private:
	static const std::array<Vertex, 8>& GetCubeVertices() noexcept;
	static const std::array<unsigned short, 36>& GetCubeIndices() noexcept;
	static const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& GetInputLayoutDesc() noexcept;
	static const char* GetSolidShaderSource() noexcept;
private:
	Graphics& gfx;
	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;
	VertexShader vertexShader;
	PixelShader pixelShader;
	InputLayout inputLayout;
	VertexConstantBuffer<TransformData> transformBuffer;
	Topology topology;
};
