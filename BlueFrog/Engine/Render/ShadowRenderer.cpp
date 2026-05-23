#include "ShadowRenderer.h"

#include "../Scene/CombatComponent.h"
#include "../Scene/SceneObject.h"

#include <array>

namespace
{
	// Unit quad on the XZ plane, vertex range [-1, 1]. World scale is
	// applied per draw via the transform cbuffer.
	const std::array<ShadowPipeline::ShadowVertex, 4>& GetQuadVertices()
	{
		static const std::array<ShadowPipeline::ShadowVertex, 4> verts =
		{
			ShadowPipeline::ShadowVertex{ -1, 0, -1,  0, 0 },
			ShadowPipeline::ShadowVertex{  1, 0, -1,  1, 0 },
			ShadowPipeline::ShadowVertex{ -1, 0,  1,  0, 1 },
			ShadowPipeline::ShadowVertex{  1, 0,  1,  1, 1 },
		};
		return verts;
	}

	const std::array<unsigned short, 6>& GetQuadIndices()
	{
		static const std::array<unsigned short, 6> indices = { 0, 2, 1, 2, 3, 1 };
		return indices;
	}

	// Y offset above the ground plane. Small enough that the shadow
	// reads as flat on the floor, large enough to avoid z-fighting
	// against the ground mesh at y=0.
	constexpr float kShadowLift = 0.02f;

	// Default footprint when an object has no CollisionComponent for
	// us to size against. Pulled out so all "no collision" actors get
	// a consistent shadow size.
	constexpr float kDefaultHalfExtent = 0.5f;
}

ShadowRenderer::ShadowRenderer(Graphics& gfx_)
	:
	gfx(gfx_),
	vertexShader(gfx_, ShadowPipeline::GetShaderSource(), "VSMain"),
	pixelShader(gfx_, ShadowPipeline::GetShaderSource(), "PSMain"),
	inputLayout(gfx_, ShadowPipeline::GetInputLayoutDesc().data(),
		static_cast<UINT>(ShadowPipeline::GetInputLayoutDesc().size()),
		vertexShader),
	vertexBuffer(gfx_, GetQuadVertices().data(),
		static_cast<UINT>(GetQuadVertices().size() * sizeof(ShadowPipeline::ShadowVertex)),
		sizeof(ShadowPipeline::ShadowVertex)),
	indexBuffer(gfx_, GetQuadIndices().data(), static_cast<UINT>(GetQuadIndices().size())),
	topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST),
	paramsBuffer(gfx_)
{
	// Standard "over" alpha blending — shadow's RGB(0,0,0) multiplied
	// by its alpha, dropped on top of whatever the lit pass wrote.
	D3D11_BLEND_DESC bd = {};
	bd.RenderTarget[0].BlendEnable = TRUE;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	if (FAILED(gfx.GetDevice()->CreateBlendState(&bd, pAlphaBlendState.GetAddressOf())))
	{
		throw BFGFX_EXCEPT(E_FAIL);
	}

	// Depth: test enabled so characters drawn above this y=0.02 layer
	// (Y is bigger ⇒ closer to top-down camera) keep their lit pixels.
	// Write disabled so the shadow itself doesn't punch a hole in the
	// depth buffer.
	D3D11_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsd.StencilEnable = FALSE;
	if (FAILED(gfx.GetDevice()->CreateDepthStencilState(&dsd, pDepthReadOnlyState.GetAddressOf())))
	{
		throw BFGFX_EXCEPT(E_FAIL);
	}
}

void ShadowRenderer::Render(const Scene& scene, const TopDownCamera& camera) noexcept
{
	using namespace DirectX;

	const XMMATRIX viewProj = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	// Bind one-time state.
	vertexShader.Bind(gfx);
	pixelShader.Bind(gfx);
	inputLayout.Bind(gfx);
	vertexBuffer.Bind(gfx);
	indexBuffer.Bind(gfx);
	topology.Bind(gfx);
	paramsBuffer.Bind(gfx, 0u);

	auto* ctx = gfx.GetContext();
	const FLOAT blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	ctx->OMSetBlendState(pAlphaBlendState.Get(), blendFactor, 0xFFFFFFFF);
	ctx->OMSetDepthStencilState(pDepthReadOnlyState.Get(), 0u);

	for (const SceneObject& obj : scene.GetObjects())
	{
		// Cast a shadow only for objects that act like "things in the
		// world" — combat-bearing actors (player, enemies, NPCs). Static
		// scenery (walls, ground, decorative cubes) typically doesn't
		// have a CombatComponent and stays shadow-free for v1.
		if (!obj.combatComponent.has_value()) continue;
		if (!obj.enabled) continue;
		// Skip dead actors so corpses don't keep their shadows after
		// their visual collapses to a flattened tint.
		if (!obj.combatComponent->IsAlive()) continue;

		// Footprint sized to the actor's collision (proxy for body
		// width). Slight 1.1× pad so the shadow extends a touch
		// beyond the bounding box, reading more like a soft cast.
		float halfX = kDefaultHalfExtent;
		float halfZ = kDefaultHalfExtent;
		if (obj.collisionComponent.has_value())
		{
			halfX = obj.collisionComponent->halfExtents.x * 1.1f;
			halfZ = obj.collisionComponent->halfExtents.y * 1.1f;
		}

		const XMMATRIX world =
			XMMatrixScaling(halfX, 1.0f, halfZ) *
			XMMatrixTranslation(obj.transform.position.x, kShadowLift, obj.transform.position.z);

		ShadowParams params;
		XMStoreFloat4x4(&params.transform, XMMatrixTranspose(world * viewProj));
		params.shadowAlpha = 0.55f;
		paramsBuffer.Update(gfx, params);

		ctx->DrawIndexed(indexBuffer.GetCount(), 0u, 0);
	}

	// Restore default blend + depth state so the next renderer in the
	// pipeline (e.g. DebugRenderer or WorldGridRenderer) sees a clean
	// context.
	ctx->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
	ctx->OMSetDepthStencilState(nullptr, 0u);
}
