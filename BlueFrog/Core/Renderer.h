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
			ImportedAnimation&& animation);

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
		ImportedAnimation            animation; // first clip; empty channels => bind pose
	};

	struct SkinningData
	{
		DirectX::XMFLOAT4X4 jointMatrices[SkinnedPipeline::MaxJoints];
	};

public:
	explicit Renderer(Graphics& gfx);
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	// `animTime` is the global animation clock in seconds. All skinned
	// meshes sample their first clip at `fmod(animTime, clip.duration)` —
	// per-instance timeline state arrives in Stage 4 (state machine).
	void Render(const Scene& scene, const TopDownCamera& camera, float animTime) noexcept;

private:
	void BindLitState() noexcept;
	void BindSkinnedState() noexcept;
	const MeshBuffers& ResolveMesh(const RenderComponent& renderComponent);
	const SkinnedMeshBuffers* ResolveSkinnedMesh(const RenderComponent& renderComponent);
	void DrawMesh(const MeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept;
	void DrawSkinnedMesh(const SkinnedMeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera, float animTime) noexcept;
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
	VertexConstantBuffer<TransformData> transformBuffer;
	PixelConstantBuffer<MaterialData> materialBuffer;
	PixelConstantBuffer<LightData> lightBuffer;
	VertexConstantBuffer<SkinningData> skinningBuffer;
	Texture2D defaultWhiteTexture;
	std::unordered_map<std::string, Texture2D> textureCache;
	Sampler samplerWrapLinear;
	Sampler samplerClampLinear;
	Sampler samplerWrapPoint;
	Topology topology;
};
