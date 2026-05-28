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
	};

	struct TransformData
	{
		DirectX::XMFLOAT4X4 mvp;
		DirectX::XMFLOAT4X4 model;
	};

	struct MaterialData
	{
		DirectX::XMFLOAT3 tint;
		float pad0 = 0.0f;
	};

	struct LightData
	{
		DirectX::XMFLOAT3 lightDir;
		float ambient = 0.0f;
		DirectX::XMFLOAT3 lightColor;
		float pad1 = 0.0f;
	};

	struct MeshBuffers
	{
		MeshBuffers(Graphics& gfx, const LitVertex* vertices, UINT vertexCount, const unsigned short* indices, UINT indexCount);

		VertexBuffer vertexBuffer;
		IndexBuffer indexBuffer;
		// Optional asset-embedded baseColorTexture. Owned per-mesh so its
		// lifetime tracks the cached imported mesh. nullptr -> Renderer
		// falls back to defaultWhiteTexture during draw.
		std::unique_ptr<Texture2D> diffuseTexture;
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
		std::unique_ptr<Texture2D>     diffuseTexture; // see MeshBuffers comment
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
	std::unordered_map<std::string, Texture2D> textureCache;
	Sampler samplerWrapLinear;
	Sampler samplerClampLinear;
	Sampler samplerWrapPoint;
	Topology topology;
};
