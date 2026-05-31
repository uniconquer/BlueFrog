#pragma once
#include "Graphics.h"
#include "../Engine/Camera/TopDownCamera.h"
#include "../Engine/Render/ConstantBuffer.h"
#include "../Engine/Render/IndexBuffer.h"
#include "../Engine/Render/InputLayout.h"
#include "../Engine/Render/LitPipeline.h"
#include "../Engine/Render/PixelShader.h"
#include "../Engine/Render/MeshImporter.h"
#include "../Engine/Render/SkinnedPipeline.h"
#include "../Engine/Render/ShadowMapPass.h"
#include "../Engine/Scene/AnimationStateComponent.h"
#include "../Engine/Render/Sampler.h"
#include "../Engine/Render/Texture2D.h"
#include "../Engine/Render/Topology.h"
#include "../Engine/Render/VertexBuffer.h"
#include "../Engine/Render/VertexShader.h"
#include "../Engine/Scene/Material.h"
#include "../Engine/Scene/RenderComponent.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/Scene/Transform.h"
#include <DirectXMath.h>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>

class Renderer
{
private:
	struct LitVertex
	{
		float x, y, z;
		float nx, ny, nz;
		float u, v;
		// Vertex color (rgba). Default white so the cube/plane primitives'
		// aggregate initializers (which only list pos/normal/uv) and any
		// untextured mesh fall back to "albedo * tint" unchanged. Colored
		// low-poly imports overwrite these per vertex.
		float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
	};

	struct TransformData
	{
		DirectX::XMFLOAT4X4 mvp;
		DirectX::XMFLOAT4X4 model;
	};

	// b1, shared by the lit (PBR) and skinned pixel shaders. tint stays the
	// first field so the skinned shader, which only declares { float3 tint;
	// float pad; }, still reads it correctly from the larger bound buffer.
	// The remaining fields drive the PBR lit shader; has* are 0/1 flags
	// telling it which maps are bound (absent maps fall back to factors).
	struct MaterialData
	{
		DirectX::XMFLOAT3 tint;
		float pad0 = 0.0f;
		DirectX::XMFLOAT4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 emissiveFactor  = { 0.0f, 0.0f, 0.0f, 0.0f }; // xyz
		float metallicFactor  = 0.0f;
		float roughnessFactor = 1.0f;
		float hasMetalRough   = 0.0f;
		float hasNormal       = 0.0f;
		float hasEmissive     = 0.0f;
		float hasOcclusion    = 0.0f;
		float hasAlbedo       = 0.0f;
		// Multiplied into the output alpha. 1.0 for normal opaque draws; the
		// placement tool sets it < 1 (with alpha blending on) to render a
		// translucent "ghost" preview of the prefab mesh at the cursor.
		float ghostAlpha      = 1.0f;
	};

	struct LightData
	{
		DirectX::XMFLOAT3 lightDir;
		float ambient = 0.0f;
		DirectX::XMFLOAT3 lightColor;
		float pad1 = 0.0f;
		// Camera world position for the PBR specular view vector. Appended
		// after the original 32B block so the skinned shader (which declares
		// only up to pad1) still reads a valid prefix.
		DirectX::XMFLOAT3 camPos;
		float pad2 = 0.0f;
		// Hemispheric ambient (B2): sky color tints upward-facing surfaces,
		// ground color the downward — blended by the world normal's Y. Gives
		// tree tops / ground a brighter natural fill and undersides more depth
		// than a single flat ambient term did.
		DirectX::XMFLOAT3 ambientSky;
		float pad3 = 0.0f;
		DirectX::XMFLOAT3 ambientGround;
		float pad4 = 0.0f;
	};

	struct MeshBuffers
	{
		MeshBuffers(Graphics& gfx, const LitVertex* vertices, UINT vertexCount, const unsigned short* indices, UINT indexCount);

		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
		// Per-material baseColorTextures + the submesh runs that reference
		// them (by index; -1 = untextured, draws with defaultWhiteTexture).
		// A single-material mesh has one texture + one submesh; a building
		// module merged from brick/wood/plaster primitives has several.
		std::vector<std::unique_ptr<Texture2D>> textures;
		std::vector<ImportedSubMesh>            submeshes;
	};

	// SkinnedMeshBuffers carries the SkinnedVertex stride (56B) plus the
	// per-mesh inverse-bind-matrix array. Joint matrices are uploaded per
	// frame by Renderer; the IBMs live here because they're a property of
	// the asset, not the per-frame pose.
	struct SkinnedMeshBuffers
	{
		SkinnedMeshBuffers(Graphics& gfx,
			const SkinnedPipeline::SkinnedVertex* vertices, UINT vertexCount,
			const unsigned short* indices, UINT indexCount,
			std::vector<DirectX::XMFLOAT4X4>&& inverseBindMatrices,
			std::vector<int>&& jointParents,
			std::vector<DirectX::XMFLOAT3>&& bindTranslation,
			std::vector<DirectX::XMFLOAT4>&& bindRotation,
			std::vector<DirectX::XMFLOAT3>&& bindScale,
			std::vector<DirectX::XMFLOAT4X4>&& jointParentBaseWorld,
			std::vector<ImportedAnimation>&& animations);

		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
		std::vector<DirectX::XMFLOAT4X4> inverseBindMatrices; // one per joint
		// Joint hierarchy + bind-pose local TRS — input to per-frame pose
		// computation. Stored here so each animated mesh draw call has all
		// the data it needs without re-reading the source ImportedMesh.
		std::vector<int>             jointParents;
		std::vector<DirectX::XMFLOAT3> bindTranslation;
		std::vector<DirectX::XMFLOAT4> bindRotation; // quaternion xyzw
		std::vector<DirectX::XMFLOAT3> bindScale;
		// Per-joint base world transform of the *non-joint* parent. Identity
		// for joints whose parent is in the joint set; non-identity for
		// joints rooted in non-joint ancestors (Armature, Z_UP, etc.).
		std::vector<DirectX::XMFLOAT4X4> jointParentBaseWorld;
		std::vector<ImportedAnimation> animations; // all clips; empty vector => bind pose
		// Multi-material textures + submesh runs (see MeshBuffers).
		std::vector<std::unique_ptr<Texture2D>> textures;
		std::vector<ImportedSubMesh>            submeshes;
	};

	struct SkinningData
	{
		DirectX::XMFLOAT4X4 jointMatrices[SkinnedPipeline::MaxJoints];
	};

	// Light view-projection for shadow sampling in the main pass (b4).
	struct ShadowData
	{
		DirectX::XMFLOAT4X4 lightViewProj;
	};

public:
	explicit Renderer(Graphics& gfx);
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	// Per-instance animation state on the SceneObject's
	// `animationStateComponent` drives skinned mesh sampling. SceneObjects
	// without that component render in bind pose. NOT noexcept so the
	// caller can catch + report any underlying mesh / texture / cgltf
	// failure instead of aborting the process via std::terminate.
	void Render(const Scene& scene, const TopDownCamera& camera);

	// Draw an imported mesh as a translucent "ghost" at the given transform
	// (placement-tool preview). Reuses the lit PBR path with alpha blending
	// + a reduced output alpha. No-op on a mesh-load failure. Call within the
	// 3D pass (it leaves the lit pipeline bound; blend state is restored).
	void DrawGhostMesh(const std::string& meshPath, float x, float y, float z,
		float yaw, float importScale, const TopDownCamera& camera) noexcept;

private:
	void BindLitState() noexcept;
	void BindSkinnedState() noexcept;
	// Shadow depth pass: render all casters from the sun's POV into the
	// shadow map. ComputeLightViewProj builds an orthographic light frustum
	// centered on the camera target so the limited top-down view always
	// has crisp shadows.
	[[nodiscard]] DirectX::XMMATRIX ComputeLightViewProj(const TopDownCamera& camera, DirectX::FXMVECTOR lightDir) const noexcept;
	void RenderShadowDepth(const Scene& scene, DirectX::FXMMATRIX lightViewProj) noexcept;
	const MeshBuffers& ResolveMesh(const RenderComponent& renderComponent);
	const SkinnedMeshBuffers* ResolveSkinnedMesh(const RenderComponent& renderComponent);
	void DrawMesh(const MeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept;
	void DrawSkinnedMesh(const SkinnedMeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera, const AnimationStateComponent* animState) noexcept;
	// Pure pose computation extracted from DrawSkinnedMesh so the shadow
	// depth pass and the main pass can share an identical skin (otherwise
	// the cast shadow would lag the visible model by a frame or pose
	// differently). No GPU/gfx access — just CPU joint math.
	SkinningData ComputeSkinningData(const SkinnedMeshBuffers& mesh, const AnimationStateComponent* animState) const noexcept;
	// Samples one clip at one time into per-joint local TRS arrays (bind
	// pose for joints the clip doesn't touch). ComputeSkinningData calls it
	// once for the active clip and, during a crossfade, a second time for
	// the outgoing clip before lerping/slerping the two poses together.
	void SamplePose(const SkinnedMeshBuffers& mesh, const std::string& clipName, float clipTime, bool looping,
		std::vector<DirectX::XMVECTOR>& outT, std::vector<DirectX::XMVECTOR>& outR, std::vector<DirectX::XMVECTOR>& outS) const noexcept;
	Texture2D& ResolveTexture(const std::string& path);
	const Sampler& ResolveSampler(SamplerPreset preset) const noexcept;
	static const std::array<LitVertex, 24>& GetCubeVertices() noexcept;
	static const std::array<unsigned short, 36>& GetCubeIndices() noexcept;
	static const std::array<LitVertex, 4>& GetPlaneVertices() noexcept;
	static const std::array<unsigned short, 6>& GetPlaneIndices() noexcept;

private:
	Graphics& gfx;
	MeshBuffers cubeMesh;
	MeshBuffers planeMesh;
	// Imported glTF meshes, keyed by source path. Loaded lazily on the first
	// frame a scene object referencing the path is rendered. Same lifetime
	// model as textureCache below — survives scene reloads.
	std::unordered_map<std::string, MeshBuffers> importedMeshCache;
	std::unordered_map<std::string, SkinnedMeshBuffers> skinnedMeshCache;
	VertexShader litVertexShader;
	PixelShader litPixelShader;
	InputLayout litInputLayout;
	VertexShader skinnedVertexShader;
	PixelShader skinnedPixelShader;
	InputLayout skinnedInputLayout;
	// Shadow map depth pass resources.
	ShadowMapPass shadowPass;
	VertexShader depthStaticVertexShader;
	InputLayout  depthStaticInputLayout;
	VertexShader depthSkinnedVertexShader;
	InputLayout  depthSkinnedInputLayout;
	VertexConstantBuffer<TransformData> transformBuffer;
	PixelConstantBuffer<MaterialData> materialBuffer;
	PixelConstantBuffer<LightData> lightBuffer;
	VertexConstantBuffer<SkinningData> skinningBuffer;
	VertexConstantBuffer<ShadowData> shadowBuffer;
	Texture2D defaultWhiteTexture;
	// Placement-ghost rendering: alpha-blend state + the per-draw alpha the
	// lit/skinned shaders multiply into their output (1.0 = opaque normal draw).
	Microsoft::WRL::ComPtr<ID3D11BlendState> ghostBlendState;
	float currentGhostAlpha = 1.0f;
	std::unordered_map<std::string, Texture2D> textureCache;
	Sampler samplerWrapLinear;
	Sampler samplerClampLinear;
	Sampler samplerWrapPoint;
	Topology topology;
};
