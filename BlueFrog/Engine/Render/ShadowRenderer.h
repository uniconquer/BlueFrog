#pragma once

#include "../../Core/Graphics.h"
#include "../Camera/TopDownCamera.h"
#include "../Scene/Scene.h"
#include "ConstantBuffer.h"
#include "IndexBuffer.h"
#include "InputLayout.h"
#include "PixelShader.h"
#include "ShadowPipeline.h"
#include "Topology.h"
#include "VertexBuffer.h"
#include "VertexShader.h"
#include <DirectXMath.h>
#include <wrl/client.h>

// Per-frame "blob shadow" pass: a soft black quad pinned to the ground
// directly under each combat-bearing scene object. The shape is the
// authored mesh's CollisionComponent.halfExtents (so the shadow
// approximates the actor's footprint), the visual is procedural
// (ShadowPipeline's pixel shader does the circular falloff). Called
// after the lit pass and before HUD overlays.
//
// Why combat-component as the "this casts a shadow" signal: every
// living/dynamic actor (player, enemies, villagers, boss) carries one,
// while static scene objects (walls, ground, decoration cubes) don't.
// Cheap, accurate-enough heuristic without an extra component for v1.
class ShadowRenderer final
{
public:
	explicit ShadowRenderer(Graphics& gfx);
	ShadowRenderer(const ShadowRenderer&) = delete;
	ShadowRenderer& operator=(const ShadowRenderer&) = delete;

	void Render(const Scene& scene, const TopDownCamera& camera) noexcept;

private:
	struct ShadowParams
	{
		DirectX::XMFLOAT4X4 transform;
		float               shadowAlpha = 0.55f;
		float               pad[3]      = { 0.0f, 0.0f, 0.0f };
	};

	Graphics&                                        gfx;
	VertexShader                                     vertexShader;
	PixelShader                                      pixelShader;
	InputLayout                                      inputLayout;
	VertexBuffer                                     vertexBuffer;
	IndexBuffer                                      indexBuffer;
	Topology                                         topology;
	VertexConstantBuffer<ShadowParams>               paramsBuffer;
	Microsoft::WRL::ComPtr<ID3D11BlendState>         pAlphaBlendState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  pDepthReadOnlyState;
};
