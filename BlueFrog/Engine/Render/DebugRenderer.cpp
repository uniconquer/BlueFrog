#include "DebugRenderer.h"

#include "../Scene/CollisionComponent.h"
#include "../Scene/SceneObject.h"
#include "../Scene/TriggerComponent.h"

namespace
{
    constexpr DirectX::XMFLOAT3 kCollisionBlockingColor = { 0.20f, 0.95f, 1.00f };  // cyan
    constexpr DirectX::XMFLOAT3 kCollisionPassThruColor = { 0.40f, 0.65f, 0.90f };  // dimmer blue
    constexpr DirectX::XMFLOAT3 kTriggerArmedColor      = { 1.00f, 0.30f, 0.95f };  // magenta
    constexpr DirectX::XMFLOAT3 kTriggerFiredColor      = { 0.45f, 0.20f, 0.45f };  // dim magenta
}

DebugRenderer::DebugRenderer(Graphics& gfxIn)
    :
    gfx(gfxIn),
    vertexShader(gfx, DebugPipeline::GetShaderSource(), "VSMain"),
    pixelShader (gfx, DebugPipeline::GetShaderSource(), "PSMain"),
    inputLayout (gfx, DebugPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(DebugPipeline::GetInputLayoutDesc().size()), vertexShader),
    viewProjBuffer(gfx),
    topologyLineList(D3D11_PRIMITIVE_TOPOLOGY_LINELIST)
{
    // Depth-disabled state so gizmos punch through walls. We restore default
    // (null) state after the gizmo draw, which means the pipeline returns to
    // the implicit default depth-on state used by the rest of the renderer —
    // matching how Renderer.cpp never explicitly sets a depth-stencil state.
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable    = FALSE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc      = D3D11_COMPARISON_ALWAYS;
    dsDesc.StencilEnable  = FALSE;
    if (const HRESULT hr = gfx.GetDevice()->CreateDepthStencilState(&dsDesc, pNoDepthState.GetAddressOf()); FAILED(hr))
    {
        throw BFGFX_EXCEPT(hr);
    }

    EnsureCapacity(1024u); // 128 boxes worth of line verts; grows if exceeded.
}

void DebugRenderer::EnsureCapacity(UINT vertexCount)
{
    if (vertexCount <= vertexBufferCapacity && pVertexBuffer)
    {
        return;
    }

    // Grow geometrically so repeated mid-frame growth doesn't thrash the
    // device. In practice the initial 1024 is more than enough.
    UINT newCapacity = vertexBufferCapacity == 0u ? 256u : vertexBufferCapacity;
    while (newCapacity < vertexCount)
    {
        newCapacity *= 2u;
    }

    D3D11_BUFFER_DESC desc = {};
    desc.BindFlags          = D3D11_BIND_VERTEX_BUFFER;
    desc.Usage              = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;
    desc.ByteWidth          = newCapacity * static_cast<UINT>(sizeof(DebugPipeline::DebugVertex));
    desc.StructureByteStride = sizeof(DebugPipeline::DebugVertex);

    Microsoft::WRL::ComPtr<ID3D11Buffer> newBuffer;
    if (const HRESULT hr = gfx.GetDevice()->CreateBuffer(&desc, nullptr, newBuffer.GetAddressOf()); FAILED(hr))
    {
        throw BFGFX_EXCEPT(hr);
    }
    pVertexBuffer        = std::move(newBuffer);
    vertexBufferCapacity = newCapacity;
}

void DebugRenderer::AppendAabbXZ(float cx, float y, float cz, float halfX, float halfZ, const DirectX::XMFLOAT3& color) noexcept
{
    // 4 edges of an XZ-plane rectangle = 4 line segments = 8 vertices.
    const float x0 = cx - halfX, x1 = cx + halfX;
    const float z0 = cz - halfZ, z1 = cz + halfZ;
    const auto V = [&](float x, float z) -> DebugPipeline::DebugVertex
    {
        return { x, y, z, color.x, color.y, color.z };
    };
    // North edge (z = z1)
    scratch.push_back(V(x0, z1)); scratch.push_back(V(x1, z1));
    // South edge (z = z0)
    scratch.push_back(V(x0, z0)); scratch.push_back(V(x1, z0));
    // East edge (x = x1)
    scratch.push_back(V(x1, z0)); scratch.push_back(V(x1, z1));
    // West edge (x = x0)
    scratch.push_back(V(x0, z0)); scratch.push_back(V(x0, z1));
}

void DebugRenderer::Render(const Scene& scene, const TopDownCamera& camera) noexcept
{
    using namespace DirectX;

    scratch.clear();

    for (const SceneObject& obj : scene.GetObjects())
    {
        const float ox = obj.transform.position.x;
        const float oy = obj.transform.position.y;
        const float oz = obj.transform.position.z;

        if (obj.collisionComponent.has_value())
        {
            const auto& cc = obj.collisionComponent.value();
            const auto& color = cc.blocksMovement ? kCollisionBlockingColor : kCollisionPassThruColor;
            AppendAabbXZ(ox, oy, oz, cc.halfExtents.x, cc.halfExtents.y, color);
        }

        if (obj.triggerComponent.has_value())
        {
            const auto& tc = obj.triggerComponent.value();
            const auto& color = tc.fired ? kTriggerFiredColor : kTriggerArmedColor;
            AppendAabbXZ(ox, oy, oz, tc.halfExtents.x, tc.halfExtents.y, color);
        }
    }

    if (scratch.empty())
    {
        return;
    }

    const UINT vertexCount = static_cast<UINT>(scratch.size());
    EnsureCapacity(vertexCount);

    // Upload via WRITE_DISCARD: we don't care about prior contents, the GPU
    // gets a fresh allocation under the hood, and the previous frame's draw
    // (if still in flight) keeps reading the old backing memory.
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (const HRESULT hr = gfx.GetContext()->Map(pVertexBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mapped); FAILED(hr))
    {
        return; // Skip this frame; gizmos are non-essential.
    }
    std::memcpy(mapped.pData, scratch.data(), vertexCount * sizeof(DebugPipeline::DebugVertex));
    gfx.GetContext()->Unmap(pVertexBuffer.Get(), 0u);

    // Bind pipeline state.
    vertexShader.Bind(gfx);
    pixelShader.Bind(gfx);
    inputLayout.Bind(gfx);
    topologyLineList.Bind(gfx);

    const XMMATRIX viewProj = camera.GetViewMatrix() * camera.GetProjectionMatrix();
    ViewProjData vpData = {};
    XMStoreFloat4x4(&vpData.viewProj, XMMatrixTranspose(viewProj));
    viewProjBuffer.Update(gfx, vpData);
    viewProjBuffer.Bind(gfx, 0u); // debug VS expects viewProj at b0

    const UINT stride = sizeof(DebugPipeline::DebugVertex);
    const UINT offset = 0u;
    ID3D11Buffer* const buffer = pVertexBuffer.Get();
    gfx.GetContext()->IASetVertexBuffers(0u, 1u, &buffer, &stride, &offset);

    gfx.GetContext()->OMSetDepthStencilState(pNoDepthState.Get(), 0u);
    gfx.GetContext()->Draw(vertexCount, 0u);
    gfx.GetContext()->OMSetDepthStencilState(nullptr, 0u);
}
