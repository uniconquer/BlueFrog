#pragma once

#include "../../Core/Graphics.h"
#include "../Render/ConstantBuffer.h"
#include "../Render/IndexBuffer.h"
#include "../Render/InputLayout.h"
#include "../Render/PixelShader.h"
#include "../Render/Topology.h"
#include "../Render/VertexBuffer.h"
#include "../Render/VertexShader.h"
#include "HealthBar.h"
#include "HudState.h"
#include <DirectXMath.h>
#include <array>

class UIRenderer
{
public:
	explicit UIRenderer(Graphics& gfx);
	UIRenderer(const UIRenderer&) = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;
	void Render(const HudState& hudState) noexcept;
	void RenderHealthBar(const HealthBar& bar) noexcept;
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
private:
	void BindSharedState() noexcept;
	void DrawQuad(const HealthBar& bar, float ratio) noexcept;
	static const std::array<Vertex, 4>& GetQuadVertices() noexcept;
	static const std::array<unsigned short, 6>& GetQuadIndices() noexcept;
	static const std::array<D3D11_INPUT_ELEMENT_DESC, 2>& GetInputLayoutDesc() noexcept;
	static const char* GetShaderSource() noexcept;
private:
	Graphics& gfx;
	MeshBuffers quadMesh;
	VertexShader vertexShader;
	PixelShader pixelShader;
	InputLayout inputLayout;
	VertexConstantBuffer<TransformData> transformBuffer;
	PixelConstantBuffer<ColorData> colorBuffer;
	Topology topology;
};
