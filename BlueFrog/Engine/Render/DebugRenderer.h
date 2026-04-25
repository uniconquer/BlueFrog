#pragma once

#include "../../Core/Graphics.h"
#include "../Camera/TopDownCamera.h"
#include "../Scene/Scene.h"
#include "ConstantBuffer.h"
#include "DebugPipeline.h"
#include "InputLayout.h"
#include "PixelShader.h"
#include "Topology.h"
#include "VertexShader.h"
#include <DirectXMath.h>
#include <vector>
#include <wrl/client.h>

// Wireframe debug overlay drawn after the lit pass. Walks the scene each call
// and emits line-list geometry for every CollisionComponent (cyan) and
// TriggerComponent (magenta) it finds. Toggled at the App level (F1 by
// default); when disabled, App simply does not call Render(), so neither the
// dynamic buffer map nor the draw call run.
//
// Depth test is disabled during the gizmo pass so lines remain visible
// through solid geometry — that is the entire point of a debug overlay.
class DebugRenderer final
{
public:
    explicit DebugRenderer(Graphics& gfx);
    DebugRenderer(const DebugRenderer&) = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;

    void Render(const Scene& scene, const TopDownCamera& camera) noexcept;

private:
    struct ViewProjData
    {
        DirectX::XMFLOAT4X4 viewProj;
    };

    void EnsureCapacity(UINT vertexCount);
    void AppendAabbXZ(float cx, float y, float cz, float halfX, float halfZ, const DirectX::XMFLOAT3& color) noexcept;

    Graphics& gfx;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  pNoDepthState;
    UINT                                             vertexBufferCapacity = 0u;
    VertexShader                                     vertexShader;
    PixelShader                                      pixelShader;
    InputLayout                                      inputLayout;
    VertexConstantBuffer<ViewProjData>               viewProjBuffer;
    Topology                                         topologyLineList;
    std::vector<DebugPipeline::DebugVertex>          scratch;
};
