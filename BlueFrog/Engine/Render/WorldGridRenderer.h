#pragma once

#include "../../Core/Graphics.h"
#include "../Camera/TopDownCamera.h"
#include "ConstantBuffer.h"
#include "DebugPipeline.h"
#include "InputLayout.h"
#include "PixelShader.h"
#include "Topology.h"
#include "VertexBuffer.h"
#include "VertexShader.h"
#include <DirectXMath.h>

// Unity-style ground-plane reference grid. Dim 1m lines spanning ±20m on the
// XZ plane, brighter every 5m, red X axis, blue Z axis. Toggled by App at
// runtime (F3 by default). Geometry is static -- built once at construction
// using DebugPipeline's pos+color line vertex format and a regular static
// vertex buffer.
//
// Drawn after the lit pass + skinned pass with depth disabled so the grid
// reads through walls / characters; that's the same convention DebugRenderer
// uses for its collision gizmos and is the more useful behavior for a
// "dimension aid" overlay.
class WorldGridRenderer final
{
public:
	explicit WorldGridRenderer(Graphics& gfx);
	WorldGridRenderer(const WorldGridRenderer&) = delete;
	WorldGridRenderer& operator=(const WorldGridRenderer&) = delete;

	void Render(const TopDownCamera& camera) noexcept;

private:
	struct ViewProjData
	{
		DirectX::XMFLOAT4X4 viewProj;
	};

	Graphics&                                        gfx;
	Microsoft::WRL::ComPtr<ID3D11Buffer>             pVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  pNoDepthState;
	UINT                                             vertexCount = 0u;
	VertexShader                                     vertexShader;
	PixelShader                                      pixelShader;
	InputLayout                                      inputLayout;
	VertexConstantBuffer<ViewProjData>               viewProjBuffer;
	Topology                                         topologyLineList;
};
