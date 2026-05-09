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

	// Single combined cbuffer for both VS (transform) and PS (tint). The
	// previous two-cbuffer setup hit a `register` collision FXC quietly
	// resolved in a way that left tint reading garbage; merging avoids
	// the register-namespace question entirely.
	struct UIConstants
	{
		DirectX::XMFLOAT4X4 transform; // 64B
		DirectX::XMFLOAT3   tint;      // 12B
		float               padding = 0.0f; // 4B (16B alignment)
	};

	struct MeshBuffers
	{
		MeshBuffers(Graphics& gfx, const Vertex* vertices, UINT vertexCount, const unsigned short* indices, UINT indexCount);

		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
	};
public:
	explicit UIRenderer(Graphics& gfx);
	UIRenderer(const UIRenderer&) = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;
	void Render(const HudState& hudState) noexcept;
private:
	void BindSharedState() noexcept;
	void DrawQuad(float centerX, float centerY, float width, float height, const DirectX::XMFLOAT3& tint) noexcept;
	void DrawBar(const HealthBar& bar) noexcept;
	void DrawCrosshair() noexcept;
	static const std::array<Vertex, 4>& GetQuadVertices() noexcept;
	static const std::array<unsigned short, 6>& GetQuadIndices() noexcept;
private:
	Graphics& gfx;
	MeshBuffers quadMesh;
	VertexShader vertexShader;
	PixelShader pixelShader;
	InputLayout inputLayout;
	// One D3D11 buffer manually bound to BOTH VS (slot 0) and PS (slot 0)
	// in BindSharedState. VS reads `transform`; PS reads `tint`. We use
	// VertexConstantBuffer as the wrapper but bind to PS too via raw
	// context calls so we avoid duplicating the underlying resource.
	VertexConstantBuffer<UIConstants> constantsBuffer;
	Topology topology;
	// Depth disabled for UI: every quad is at NDC z=0 so under the default
	// `LESS` depth func, the second draw at the same z fails the test
	// against the first draw's z value -- producing the no-fill bars the
	// user reported. Disabling depth for the entire UI pass is correct
	// since UI elements are explicitly draw-order-layered.
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> pNoDepthState;
};
