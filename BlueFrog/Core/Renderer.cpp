#include "Renderer.h"
#include "../Engine/Render/ImageLoader.h"
#include "../Engine/Render/MeshImporter.h"
#include <DirectXMath.h>
#include <cmath>
#include <filesystem>
#include <stdexcept>

Renderer::MeshBuffers::MeshBuffers(Graphics& gfx, const LitVertex* vertices, UINT vertexCount, const unsigned short* indices, UINT indexCount)
	:
	vertexBuffer(gfx, vertices, vertexCount * static_cast<UINT>(sizeof(LitVertex)), sizeof(LitVertex)),
	indexBuffer(gfx, indices, indexCount)
{
}

Renderer::SkinnedMeshBuffers::SkinnedMeshBuffers(
	Graphics& gfx,
	const SkinnedPipeline::SkinnedVertex* vertices, UINT vertexCount,
	const unsigned short* indices, UINT indexCount,
	std::vector<DirectX::XMFLOAT4X4>&& ibms,
	std::vector<int>&& parents,
	std::vector<DirectX::XMFLOAT3>&& bindT,
	std::vector<DirectX::XMFLOAT4>&& bindR,
	std::vector<DirectX::XMFLOAT3>&& bindS,
	ImportedAnimation&& anim)
	:
	vertexBuffer(gfx, vertices, vertexCount * static_cast<UINT>(sizeof(SkinnedPipeline::SkinnedVertex)), sizeof(SkinnedPipeline::SkinnedVertex)),
	indexBuffer(gfx, indices, indexCount),
	inverseBindMatrices(std::move(ibms)),
	jointParents(std::move(parents)),
	bindTranslation(std::move(bindT)),
	bindRotation(std::move(bindR)),
	bindScale(std::move(bindS)),
	animation(std::move(anim))
{
}

static Surface MakeWhiteSurface()
{
	Surface s(1u, 1u);
	std::uint8_t* const p = s.GetPixels();
	p[0] = p[1] = p[2] = p[3] = 255u;
	return s;
}

Renderer::Renderer(Graphics& gfx)
	:
	gfx(gfx),
	cubeMesh(gfx, GetCubeVertices().data(), static_cast<UINT>(GetCubeVertices().size()), GetCubeIndices().data(), static_cast<UINT>(GetCubeIndices().size())),
	planeMesh(gfx, GetPlaneVertices().data(), static_cast<UINT>(GetPlaneVertices().size()), GetPlaneIndices().data(), static_cast<UINT>(GetPlaneIndices().size())),
	litVertexShader(gfx, LitPipeline::GetShaderSource(), "VSMain"),
	litPixelShader(gfx, LitPipeline::GetShaderSource(), "PSMain"),
	litInputLayout(gfx, LitPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(LitPipeline::GetInputLayoutDesc().size()), litVertexShader),
	skinnedVertexShader(gfx, SkinnedPipeline::GetShaderSource(), "VSMain"),
	skinnedPixelShader(gfx, SkinnedPipeline::GetShaderSource(), "PSMain"),
	skinnedInputLayout(gfx, SkinnedPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(SkinnedPipeline::GetInputLayoutDesc().size()), skinnedVertexShader),
	transformBuffer(gfx),
	materialBuffer(gfx),
	lightBuffer(gfx),
	skinningBuffer(gfx),
	defaultWhiteTexture(gfx, MakeWhiteSurface()),
	samplerWrapLinear(gfx),
	samplerClampLinear(gfx, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP),
	samplerWrapPoint(gfx, D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP),
	topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{
}

const std::array<Renderer::LitVertex, 24>& Renderer::GetCubeVertices() noexcept
{
	static const std::array<LitVertex, 24> vertices =
	{
		// Back (-Z), normal (0,0,-1)
		LitVertex{ -1,-1,-1,  0, 0,-1,  0,1 },
		LitVertex{  1,-1,-1,  0, 0,-1,  1,1 },
		LitVertex{ -1, 1,-1,  0, 0,-1,  0,0 },
		LitVertex{  1, 1,-1,  0, 0,-1,  1,0 },
		// Front (+Z), normal (0,0,+1)
		LitVertex{ -1,-1, 1,  0, 0, 1,  0,1 },
		LitVertex{  1,-1, 1,  0, 0, 1,  1,1 },
		LitVertex{ -1, 1, 1,  0, 0, 1,  0,0 },
		LitVertex{  1, 1, 1,  0, 0, 1,  1,0 },
		// Right (+X), normal (1,0,0)
		LitVertex{  1,-1,-1,  1, 0, 0,  0,1 },
		LitVertex{  1,-1, 1,  1, 0, 0,  1,1 },
		LitVertex{  1, 1,-1,  1, 0, 0,  0,0 },
		LitVertex{  1, 1, 1,  1, 0, 0,  1,0 },
		// Left (-X), normal (-1,0,0)
		LitVertex{ -1,-1, 1, -1, 0, 0,  0,1 },
		LitVertex{ -1,-1,-1, -1, 0, 0,  1,1 },
		LitVertex{ -1, 1, 1, -1, 0, 0,  0,0 },
		LitVertex{ -1, 1,-1, -1, 0, 0,  1,0 },
		// Top (+Y), normal (0,1,0)
		LitVertex{ -1, 1,-1,  0, 1, 0,  0,1 },
		LitVertex{  1, 1,-1,  0, 1, 0,  1,1 },
		LitVertex{ -1, 1, 1,  0, 1, 0,  0,0 },
		LitVertex{  1, 1, 1,  0, 1, 0,  1,0 },
		// Bottom (-Y), normal (0,-1,0)
		LitVertex{ -1,-1,-1,  0,-1, 0,  0,1 },
		LitVertex{  1,-1,-1,  0,-1, 0,  1,1 },
		LitVertex{ -1,-1, 1,  0,-1, 0,  0,0 },
		LitVertex{  1,-1, 1,  0,-1, 0,  1,0 },
	};
	return vertices;
}

const std::array<unsigned short, 36>& Renderer::GetCubeIndices() noexcept
{
	static const std::array<unsigned short, 36> indices =
	{
		// Back (-Z): winding 0,2,1  2,3,1
		 0, 2, 1,   2, 3, 1,
		// Front (+Z): winding 0,1,2  1,3,2
		 4, 5, 6,   5, 7, 6,
		// Right (+X): winding 0,2,1  2,3,1
		 8,10, 9,  10,11, 9,
		// Left (-X): winding 0,2,1  2,3,1
		12,14,13,  14,15,13,
		// Top (+Y): winding 0,2,1  2,3,1
		16,18,17,  18,19,17,
		// Bottom (-Y): winding 0,1,2  1,3,2
		20,21,22,  21,23,22,
	};
	return indices;
}

const std::array<Renderer::LitVertex, 4>& Renderer::GetPlaneVertices() noexcept
{
	static const std::array<LitVertex, 4> vertices =
	{
		LitVertex{ -1,0,-1,  0,1,0,  0,9 },
		LitVertex{  1,0,-1,  0,1,0,  9,9 },
		LitVertex{ -1,0, 1,  0,1,0,  0,0 },
		LitVertex{  1,0, 1,  0,1,0,  9,0 },
	};
	return vertices;
}

const std::array<unsigned short, 6>& Renderer::GetPlaneIndices() noexcept
{
	static const std::array<unsigned short, 6> indices =
	{
		0, 2, 1,
		2, 3, 1,
	};
	return indices;
}

void Renderer::BindLitState() noexcept
{
	litInputLayout.Bind(gfx);
	topology.Bind(gfx);
	litVertexShader.Bind(gfx);
	transformBuffer.Bind(gfx);
	materialBuffer.Bind(gfx, 1u);
	lightBuffer.Bind(gfx, 2u);
	litPixelShader.Bind(gfx);
}

void Renderer::BindSkinnedState() noexcept
{
	skinnedInputLayout.Bind(gfx);
	topology.Bind(gfx);
	skinnedVertexShader.Bind(gfx);
	transformBuffer.Bind(gfx);
	materialBuffer.Bind(gfx, 1u);
	lightBuffer.Bind(gfx, 2u);
	// b3 = skinning palette (joint matrices). Lit pipeline ignores b3, so
	// leaving the skinning cbuffer bound between draws is safe — the lit VS
	// does not reference it.
	skinningBuffer.Bind(gfx, 3u);
	skinnedPixelShader.Bind(gfx);
}

const Renderer::MeshBuffers& Renderer::ResolveMesh(const RenderComponent& renderComponent)
{
	if (renderComponent.meshType == RenderMeshType::External)
	{
		const std::string& path = renderComponent.meshPath;
		auto it = importedMeshCache.find(path);
		if (it != importedMeshCache.end())
		{
			return it->second;
		}

		// First reference: load + convert + upload. Failure throws so the
		// asset validator's path-prefixed error matches the engine's other
		// "missing asset" failure modes; bad meshes are caught at boot.
		ImportedMesh imported;
		std::string err;
		if (!MeshImporter::Load(std::filesystem::path(path), imported, &err))
		{
			throw std::runtime_error("MeshImporter failed: " + err);
		}

		const std::size_t vertexCount = imported.positions.size() / 3;
		std::vector<LitVertex> verts(vertexCount);
		for (std::size_t i = 0; i < vertexCount; ++i)
		{
			verts[i].x = imported.positions[i * 3 + 0];
			verts[i].y = imported.positions[i * 3 + 1];
			verts[i].z = imported.positions[i * 3 + 2];
			if (imported.normals.size() >= (i + 1) * 3)
			{
				verts[i].nx = imported.normals[i * 3 + 0];
				verts[i].ny = imported.normals[i * 3 + 1];
				verts[i].nz = imported.normals[i * 3 + 2];
			}
			else
			{
				// glTF allows omitting normals; pick (0,1,0) so the lit pass
				// still produces a non-black surface.
				verts[i].nx = 0.0f; verts[i].ny = 1.0f; verts[i].nz = 0.0f;
			}
			if (imported.uvs.size() >= (i + 1) * 2)
			{
				verts[i].u = imported.uvs[i * 2 + 0];
				verts[i].v = imported.uvs[i * 2 + 1];
			}
			else
			{
				verts[i].u = 0.0f; verts[i].v = 0.0f;
			}
		}

		auto [emplacedIt, ok] = importedMeshCache.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(path),
			std::forward_as_tuple(
				gfx,
				verts.data(),
				static_cast<UINT>(verts.size()),
				imported.indices.data(),
				static_cast<UINT>(imported.indices.size())));
		return emplacedIt->second;
	}

	switch (renderComponent.meshType)
	{
	case RenderMeshType::Plane: return planeMesh;
	case RenderMeshType::Cube:
	default:                    return cubeMesh;
	}
}

void Renderer::DrawMesh(const MeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera) noexcept
{
	using namespace DirectX;

	const XMMATRIX modelMatrix = transform.GetMatrix();
	const XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	TransformData transformData = {};
	XMStoreFloat4x4(&transformData.mvp,   XMMatrixTranspose(modelMatrix * viewProjection));
	XMStoreFloat4x4(&transformData.model, XMMatrixTranspose(modelMatrix));
	transformBuffer.Update(gfx, transformData);

	const Material mat = renderComponent.material.value_or(Material{});
	const MaterialData materialData = { mat.tint, 0.0f };
	materialBuffer.Update(gfx, materialData);

	ResolveTexture(mat.texturePath).Bind(gfx);
	ResolveSampler(mat.sampler).Bind(gfx);

	mesh.vertexBuffer.Bind(gfx);
	mesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(mesh.indexBuffer.GetCount());
}

Texture2D& Renderer::ResolveTexture(const std::string& path)
{
	if (path.empty())
	{
		return defaultWhiteTexture;
	}
	auto it = textureCache.find(path);
	if (it != textureCache.end())
	{
		return it->second;
	}
	Surface surface = ImageLoader::LoadSurfaceFromFile(std::wstring(path.begin(), path.end()));
	auto [inserted_it, ok] = textureCache.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(path),
		std::forward_as_tuple(gfx, surface));
	return inserted_it->second;
}

const Sampler& Renderer::ResolveSampler(SamplerPreset preset) const noexcept
{
	switch (preset)
	{
	case SamplerPreset::ClampLinear: return samplerClampLinear;
	case SamplerPreset::WrapPoint:   return samplerWrapPoint;
	case SamplerPreset::WrapLinear:
	default:                          return samplerWrapLinear;
	}
}

void Renderer::Render(const Scene& scene, const TopDownCamera& camera, float animTime) noexcept
{
	using namespace DirectX;

	// Upload light data once per frame (uniform scale assumed, so model matrix
	// upper-left 3x3 is a valid normal matrix without inverse-transpose).
	const XMVECTOR rawDir = XMVector3Normalize(XMVectorSet(0.3f, -1.0f, 0.2f, 0.0f));
	LightData lightData = {};
	XMStoreFloat3(&lightData.lightDir,   rawDir);
	lightData.ambient    = 0.35f;
	lightData.lightColor = { 1.0f, 0.96f, 0.90f };
	lightBuffer.Update(gfx, lightData);

	// Two-pass split: lit (static) first, skinned second. Each pass binds
	// its own pipeline state once. ResolveSkinnedMesh returns nullptr when
	// the asset has no skin data — those fall through to the lit pass.
	BindLitState();
	for (const auto& object : scene.GetObjects())
	{
		if (!object.CanRender()) continue;
		// Skip skinned meshes here; they are drawn in the second pass with
		// the matching pipeline.
		if (ResolveSkinnedMesh(*object.renderComponent) != nullptr) continue;
		DrawMesh(ResolveMesh(*object.renderComponent), object.transform, *object.renderComponent, camera);
	}

	BindSkinnedState();
	for (const auto& object : scene.GetObjects())
	{
		if (!object.CanRender()) continue;
		const SkinnedMeshBuffers* skinned = ResolveSkinnedMesh(*object.renderComponent);
		if (skinned == nullptr) continue;
		DrawSkinnedMesh(*skinned, object.transform, *object.renderComponent, camera, animTime);
	}
}

const Renderer::SkinnedMeshBuffers* Renderer::ResolveSkinnedMesh(const RenderComponent& renderComponent)
{
	if (renderComponent.meshType != RenderMeshType::External)
	{
		return nullptr;
	}
	const std::string& path = renderComponent.meshPath;

	// Negative-cache via a sentinel: once we determine a path's mesh is
	// not skinned we want subsequent calls to short-circuit. We use the
	// presence in importedMeshCache as that signal: if the path is in the
	// static cache it's been classified static. If neither cache has it,
	// classify on first hit.
	auto skinnedIt = skinnedMeshCache.find(path);
	if (skinnedIt != skinnedMeshCache.end())
	{
		return &skinnedIt->second;
	}
	if (importedMeshCache.find(path) != importedMeshCache.end())
	{
		return nullptr; // already classified static
	}

	// First reference: load and classify.
	ImportedMesh imported;
	std::string err;
	if (!MeshImporter::Load(std::filesystem::path(path), imported, &err))
	{
		throw std::runtime_error("MeshImporter failed: " + err);
	}

	if (!imported.IsSkinned())
	{
		// Hand off to ResolveMesh path which will re-load and cache as
		// static. The duplicate parse on first reference is acceptable
		// for now — Stage 2 has 2 imported assets total.
		return nullptr;
	}

	using namespace DirectX;

	const std::size_t vertexCount = imported.positions.size() / 3;
	std::vector<SkinnedPipeline::SkinnedVertex> verts(vertexCount);
	for (std::size_t i = 0; i < vertexCount; ++i)
	{
		auto& v = verts[i];
		v.x = imported.positions[i * 3 + 0];
		v.y = imported.positions[i * 3 + 1];
		v.z = imported.positions[i * 3 + 2];
		if (imported.normals.size() >= (i + 1) * 3)
		{
			v.nx = imported.normals[i * 3 + 0];
			v.ny = imported.normals[i * 3 + 1];
			v.nz = imported.normals[i * 3 + 2];
		}
		else { v.nx = 0.0f; v.ny = 1.0f; v.nz = 0.0f; }
		if (imported.uvs.size() >= (i + 1) * 2)
		{
			v.u = imported.uvs[i * 2 + 0];
			v.v = imported.uvs[i * 2 + 1];
		}
		else { v.u = 0.0f; v.v = 0.0f; }
		v.j0 = imported.jointIndices[i * 4 + 0];
		v.j1 = imported.jointIndices[i * 4 + 1];
		v.j2 = imported.jointIndices[i * 4 + 2];
		v.j3 = imported.jointIndices[i * 4 + 3];
		v.w0 = imported.jointWeights[i * 4 + 0];
		v.w1 = imported.jointWeights[i * 4 + 1];
		v.w2 = imported.jointWeights[i * 4 + 2];
		v.w3 = imported.jointWeights[i * 4 + 3];
	}

	// IBMs from cgltf are column-major in memory. DirectXMath's XMFLOAT4X4
	// is row-major, so we load the floats as a column-major XMMATRIX
	// (XMMatrixSet would require manual transpose). XMLoadFloat4x4 +
	// transpose is the cleanest pivot point.
	std::vector<XMFLOAT4X4> ibmsRowMajor(imported.jointCount);
	for (std::uint32_t j = 0; j < imported.jointCount; ++j)
	{
		// Reinterpret the 16 floats as a column-major matrix, then
		// transpose into our row-major storage so the shader sees the
		// matrix in the same column-major-as-memory layout HLSL expects
		// (HLSL `matrix` is column-major when you upload row-major +
		// transpose, matching the LitPipeline convention).
		const float* src = imported.inverseBindMatrices.data() + j * 16;
		XMFLOAT4X4 colMajor;
		std::memcpy(&colMajor, src, sizeof(XMFLOAT4X4));
		const XMMATRIX m = XMMatrixTranspose(XMLoadFloat4x4(&colMajor));
		XMStoreFloat4x4(&ibmsRowMajor[j], m);
	}

	// Repack TRS bind locals into XMFLOAT3/4 arrays (Renderer's preferred
	// layout) — the importer stored them as flat float streams.
	std::vector<XMFLOAT3> bindT(imported.jointCount);
	std::vector<XMFLOAT4> bindR(imported.jointCount);
	std::vector<XMFLOAT3> bindS(imported.jointCount);
	for (std::uint32_t j = 0; j < imported.jointCount; ++j)
	{
		bindT[j] = { imported.jointBindTranslation[j*3+0], imported.jointBindTranslation[j*3+1], imported.jointBindTranslation[j*3+2] };
		bindR[j] = { imported.jointBindRotation[j*4+0], imported.jointBindRotation[j*4+1], imported.jointBindRotation[j*4+2], imported.jointBindRotation[j*4+3] };
		bindS[j] = { imported.jointBindScale[j*3+0], imported.jointBindScale[j*3+1], imported.jointBindScale[j*3+2] };
	}

	auto [it, ok] = skinnedMeshCache.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(path),
		std::forward_as_tuple(
			gfx,
			verts.data(),
			static_cast<UINT>(verts.size()),
			imported.indices.data(),
			static_cast<UINT>(imported.indices.size()),
			std::move(ibmsRowMajor),
			std::move(imported.jointParents),
			std::move(bindT),
			std::move(bindR),
			std::move(bindS),
			std::move(imported.animation)));
	return &it->second;
}

namespace
{
	// Find the keyframe segment [k, k+1] containing time t. Returns left
	// keyframe index and segment alpha in [0, 1]. Times array must have at
	// least one entry; caller ensures.
	void FindSegment(const std::vector<float>& times, float t, std::size_t& outIdx, float& outAlpha)
	{
		if (t <= times.front()) { outIdx = 0; outAlpha = 0.0f; return; }
		if (t >= times.back())
		{
			outIdx = times.size() - 1;
			outAlpha = 0.0f; // pinned to last keyframe
			return;
		}
		// Linear search — animation channels typically have <100 keys, and
		// the alternative (binary search) is unwarranted complexity here.
		for (std::size_t i = 0; i + 1 < times.size(); ++i)
		{
			if (t >= times[i] && t < times[i + 1])
			{
				const float span = times[i + 1] - times[i];
				outIdx = i;
				outAlpha = (span > 0.0f) ? (t - times[i]) / span : 0.0f;
				return;
			}
		}
		outIdx = times.size() - 1;
		outAlpha = 0.0f;
	}
}

void Renderer::DrawSkinnedMesh(const SkinnedMeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera, float animTime) noexcept
{
	using namespace DirectX;

	const XMMATRIX modelMatrix = transform.GetMatrix();
	const XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	TransformData transformData = {};
	XMStoreFloat4x4(&transformData.mvp,   XMMatrixTranspose(modelMatrix * viewProjection));
	XMStoreFloat4x4(&transformData.model, XMMatrixTranspose(modelMatrix));
	transformBuffer.Update(gfx, transformData);

	const Material mat = renderComponent.material.value_or(Material{});
	const MaterialData materialData = { mat.tint, 0.0f };
	materialBuffer.Update(gfx, materialData);

	// === Per-frame pose computation ===========================================
	// 1. Start each joint at its bind-pose local TRS.
	// 2. For every animation channel targeting this joint, sample at
	//    `animTime mod clipDuration` and override the matching component.
	// 3. Compose local matrix = scale * rotation * translation (row-vector
	//    order: vec * (S*R*T) = scale-then-rotate-then-translate).
	// 4. Walk the joint hierarchy (parents already topologically before
	//    children in glTF skin->joints) computing world = local * parentWorld.
	// 5. jointMatrix[i] = inverseBindMatrix[i] * jointWorld[i]   (row-vector
	//    convention; in column-vector spec terms: jointMatrix = jointWorld * IBM).
	// 6. Transpose before upload so HLSL's column-major default reads
	//    consistently with the lit pipeline's transform/model upload.
	const std::uint32_t jointCount = static_cast<std::uint32_t>(mesh.inverseBindMatrices.size());

	// Sample time: loop the clip if it has nonzero duration, otherwise stay
	// at t=0 (effectively bind pose).
	const bool hasClip = !mesh.animation.channels.empty() && mesh.animation.duration > 0.0f;
	const float t = hasClip ? std::fmod(animTime, mesh.animation.duration) : 0.0f;

	// Per-joint TRS, initialized to bind. Channel sampling overrides
	// individual components; joints not addressed by any channel keep
	// their full bind TRS.
	std::vector<XMVECTOR> Tvec(jointCount);
	std::vector<XMVECTOR> Rvec(jointCount);
	std::vector<XMVECTOR> Svec(jointCount);
	for (std::uint32_t i = 0; i < jointCount; ++i)
	{
		Tvec[i] = XMLoadFloat3(&mesh.bindTranslation[i]);
		Rvec[i] = XMLoadFloat4(&mesh.bindRotation[i]);
		Svec[i] = XMLoadFloat3(&mesh.bindScale[i]);
	}

	if (hasClip)
	{
		for (const auto& ch : mesh.animation.channels)
		{
			if (ch.targetJoint < 0 || static_cast<std::uint32_t>(ch.targetJoint) >= jointCount) continue;
			if (ch.times.empty()) continue;

			std::size_t k = 0;
			float alpha = 0.0f;
			FindSegment(ch.times, t, k, alpha);
			const std::size_t k1 = (k + 1 < ch.times.size()) ? (k + 1) : k;

			const bool useStep = (ch.interpolation == ImportedAnimationChannel::Interpolation::Step);

			switch (ch.path)
			{
			case ImportedAnimationChannel::Path::Translation:
			{
				const XMVECTOR v0 = XMVectorSet(ch.values[k *3+0], ch.values[k *3+1], ch.values[k *3+2], 0.0f);
				const XMVECTOR v1 = XMVectorSet(ch.values[k1*3+0], ch.values[k1*3+1], ch.values[k1*3+2], 0.0f);
				Tvec[ch.targetJoint] = useStep ? v0 : XMVectorLerp(v0, v1, alpha);
				break;
			}
			case ImportedAnimationChannel::Path::Scale:
			{
				const XMVECTOR v0 = XMVectorSet(ch.values[k *3+0], ch.values[k *3+1], ch.values[k *3+2], 0.0f);
				const XMVECTOR v1 = XMVectorSet(ch.values[k1*3+0], ch.values[k1*3+1], ch.values[k1*3+2], 0.0f);
				Svec[ch.targetJoint] = useStep ? v0 : XMVectorLerp(v0, v1, alpha);
				break;
			}
			case ImportedAnimationChannel::Path::Rotation:
			{
				// Quaternion: slerp for smoothness, step on STEP interp.
				const XMVECTOR q0 = XMVectorSet(ch.values[k *4+0], ch.values[k *4+1], ch.values[k *4+2], ch.values[k *4+3]);
				const XMVECTOR q1 = XMVectorSet(ch.values[k1*4+0], ch.values[k1*4+1], ch.values[k1*4+2], ch.values[k1*4+3]);
				Rvec[ch.targetJoint] = useStep ? q0 : XMQuaternionSlerp(q0, q1, alpha);
				break;
			}
			}
		}
	}

	// Compose local matrices, then walk hierarchy for world matrices.
	std::vector<XMMATRIX> jointWorld(jointCount);
	for (std::uint32_t i = 0; i < jointCount; ++i)
	{
		const XMMATRIX S = XMMatrixScalingFromVector(Svec[i]);
		const XMMATRIX R = XMMatrixRotationQuaternion(Rvec[i]);
		const XMMATRIX T = XMMatrixTranslationFromVector(Tvec[i]);
		const XMMATRIX local = S * R * T;
		const int parent = (i < mesh.jointParents.size()) ? mesh.jointParents[i] : -1;
		jointWorld[i] = (parent < 0) ? local : (local * jointWorld[parent]);
	}

	SkinningData skinData = {};
	const std::uint32_t count = std::min<std::uint32_t>(jointCount, SkinnedPipeline::MaxJoints);
	for (std::uint32_t i = 0; i < count; ++i)
	{
		const XMMATRIX ibm = XMLoadFloat4x4(&mesh.inverseBindMatrices[i]);
		// Row-vector convention: pos * jointMatrix = pos * IBM * jointWorld.
		// In DirectXMath multiply order this is `IBM * jointWorld`.
		const XMMATRIX jm = ibm * jointWorld[i];
		XMStoreFloat4x4(&skinData.jointMatrices[i], XMMatrixTranspose(jm));
	}
	skinningBuffer.Update(gfx, skinData);

	ResolveTexture(mat.texturePath).Bind(gfx);
	ResolveSampler(mat.sampler).Bind(gfx);

	mesh.vertexBuffer.Bind(gfx);
	mesh.indexBuffer.Bind(gfx);
	gfx.DrawIndexed(mesh.indexBuffer.GetCount());
}
