#include "Renderer.h"
#include "../Engine/Render/ImageLoader.h"
#include "../Engine/Render/MeshImporter.h"
#include "../Engine/Render/ShadowDepthPipeline.h"
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
	std::vector<DirectX::XMFLOAT4X4>&& jpbw,
	std::vector<ImportedAnimation>&& anims)
	:
	vertexBuffer(gfx, vertices, vertexCount * static_cast<UINT>(sizeof(SkinnedPipeline::SkinnedVertex)), sizeof(SkinnedPipeline::SkinnedVertex)),
	indexBuffer(gfx, indices, indexCount),
	inverseBindMatrices(std::move(ibms)),
	jointParents(std::move(parents)),
	bindTranslation(std::move(bindT)),
	bindRotation(std::move(bindR)),
	bindScale(std::move(bindS)),
	jointParentBaseWorld(std::move(jpbw)),
	animations(std::move(anims))
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
	shadowPass(gfx),
	depthStaticVertexShader(gfx, ShadowDepthPipeline::GetStaticShaderSource(), "VSMain"),
	depthStaticInputLayout(gfx, LitPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(LitPipeline::GetInputLayoutDesc().size()), depthStaticVertexShader),
	depthSkinnedVertexShader(gfx, ShadowDepthPipeline::GetSkinnedShaderSource(), "VSMain"),
	depthSkinnedInputLayout(gfx, SkinnedPipeline::GetInputLayoutDesc().data(), static_cast<UINT>(SkinnedPipeline::GetInputLayoutDesc().size()), depthSkinnedVertexShader),
	transformBuffer(gfx),
	materialBuffer(gfx),
	lightBuffer(gfx),
	skinningBuffer(gfx),
	shadowBuffer(gfx),
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
	// Square 9×9 UV. The previous 9×7 asymmetry was a misdiagnosis —
	// the apparent rectangular checker was actually a PowerShell
	// banker's rounding bug in the texture generator (cells came out
	// 5px+7px instead of 8+8). Texture now correct; UV stays square.
	// Top-down perspective foreshortening still applies but is gentle
	// enough that a square mesh + square texture reads square.
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
	shadowBuffer.Bind(gfx, 4u); // b4 = light view-proj for shadow sampling
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
	shadowBuffer.Bind(gfx, 4u); // b4 = light view-proj for shadow sampling
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
			// Vertex color (rgba stride 4). Absent => leave the LitVertex
			// default white, so textured/untinted meshes are unaffected.
			if (imported.colors.size() >= (i + 1) * 4)
			{
				verts[i].r = imported.colors[i * 4 + 0];
				verts[i].g = imported.colors[i * 4 + 1];
				verts[i].b = imported.colors[i * 4 + 2];
				verts[i].a = imported.colors[i * 4 + 3];
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
		// Decode each imported material texture. Failure on any one is
		// non-fatal: a missing/corrupt texture leaves a null slot and that
		// submesh draws white rather than blocking scene boot.
		for (const auto& it : imported.textures)
		{
			std::unique_ptr<Texture2D> tex;
			if (!it.bytes.empty())
			{
				try
				{
					Surface surf = ImageLoader::LoadSurfaceFromMemory(it.bytes.data(), it.bytes.size());
					tex = std::make_unique<Texture2D>(gfx, surf);
				}
				catch (const std::exception&) {}
			}
			emplacedIt->second.textures.push_back(std::move(tex));
		}
		emplacedIt->second.submeshes = imported.submeshes;
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

	// Asset-level import scale is applied INSIDE the transform's local
	// space (before rotation+translation) so it behaves like a baked
	// mesh resize rather than a world-space scale. transform.scale
	// stays meaningful as "intentional in-game size".
	const float is = renderComponent.importScale;
	const XMMATRIX modelMatrix = XMMatrixScaling(is, is, is) * transform.GetMatrix();
	const XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	TransformData transformData = {};
	XMStoreFloat4x4(&transformData.mvp,   XMMatrixTranspose(modelMatrix * viewProjection));
	XMStoreFloat4x4(&transformData.model, XMMatrixTranspose(modelMatrix));
	transformBuffer.Update(gfx, transformData);

	const Material mat = renderComponent.material.value_or(Material{});
	const MaterialData materialData = { mat.tint, 0.0f };
	materialBuffer.Update(gfx, materialData);

	ResolveSampler(mat.sampler).Bind(gfx);
	mesh.vertexBuffer.Bind(gfx);
	mesh.indexBuffer.Bind(gfx);

	if (mesh.submeshes.empty())
	{
		// Hand-authored cube/plane primitives (no source asset): the
		// scene-side material.texturePath drives the texture (Ground
		// checker, etc.), falling back to white when unset.
		ResolveTexture(mat.texturePath).Bind(gfx);
		gfx.DrawIndexed(mesh.indexBuffer.GetCount());
	}
	else
	{
		// Imported mesh: draw each material run with its own texture.
		// textureIndex -1 (or a null/failed decode) binds white, so an
		// untextured / vertex-colored run shows albedo*vertexColor*tint.
		for (const auto& sub : mesh.submeshes)
		{
			Texture2D* tex = nullptr;
			if (sub.textureIndex >= 0 && sub.textureIndex < static_cast<int>(mesh.textures.size()))
			{
				tex = mesh.textures[sub.textureIndex].get();
			}
			if (tex) tex->Bind(gfx);
			else     defaultWhiteTexture.Bind(gfx);
			gfx.GetContext()->DrawIndexed(sub.indexCount, sub.indexOffset, 0);
		}
	}
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

DirectX::XMMATRIX Renderer::ComputeLightViewProj(const TopDownCamera& camera, DirectX::FXMVECTOR lightDir) const noexcept
{
	using namespace DirectX;

	// Center the light frustum on what the camera is looking at, so the
	// limited top-down view always falls inside the shadow map. Back the
	// light off along -lightDir and look at the target.
	const XMFLOAT3 tgt = camera.GetTarget();
	const XMVECTOR center = XMLoadFloat3(&tgt);
	const XMVECTOR dir = XMVector3Normalize(lightDir);

	constexpr float kLightDistance = 40.0f; // how far "up" the sun sits
	const XMVECTOR lightPos = XMVectorSubtract(center, XMVectorScale(dir, kLightDistance));
	const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // dir has X/Z tilt so never parallel
	const XMMATRIX view = XMMatrixLookAtLH(lightPos, center, up);

	// Orthographic (parallel sun rays). Width/height cover the visible
	// play area around the camera target; near/far bracket the 40-unit
	// stand-off plus scene depth.
	constexpr float kOrthoSize = 34.0f;
	const XMMATRIX proj = XMMatrixOrthographicLH(kOrthoSize, kOrthoSize, 1.0f, 90.0f);
	return view * proj;
}

void Renderer::RenderShadowDepth(const Scene& scene, DirectX::FXMMATRIX lightViewProj) noexcept
{
	using namespace DirectX;

	shadowPass.Begin(gfx);

	auto* ctx = gfx.GetContext();
	// Depth-only: no pixel shader bound. Rasterizer fills the depth buffer
	// directly from SV_Position.
	ctx->PSSetShader(nullptr, nullptr, 0u);
	topology.Bind(gfx);
	transformBuffer.Bind(gfx, 0u);

	// --- Static casters ---
	depthStaticVertexShader.Bind(gfx);
	depthStaticInputLayout.Bind(gfx);
	for (const auto& object : scene.GetObjects())
	{
		if (!object.CanRender()) continue;
		if (ResolveSkinnedMesh(*object.renderComponent) != nullptr) continue;
		const MeshBuffers& mesh = ResolveMesh(*object.renderComponent);
		const float is = object.renderComponent->importScale;
		const XMMATRIX model = XMMatrixScaling(is, is, is) * object.transform.GetMatrix();
		TransformData td = {};
		XMStoreFloat4x4(&td.mvp,   XMMatrixTranspose(model * lightViewProj));
		XMStoreFloat4x4(&td.model, XMMatrixTranspose(model));
		transformBuffer.Update(gfx, td);
		mesh.vertexBuffer.Bind(gfx);
		mesh.indexBuffer.Bind(gfx);
		gfx.DrawIndexed(mesh.indexBuffer.GetCount());
	}

	// --- Skinned casters ---
	depthSkinnedVertexShader.Bind(gfx);
	depthSkinnedInputLayout.Bind(gfx);
	skinningBuffer.Bind(gfx, 3u);
	for (const auto& object : scene.GetObjects())
	{
		if (!object.CanRender()) continue;
		const SkinnedMeshBuffers* skinned = ResolveSkinnedMesh(*object.renderComponent);
		if (skinned == nullptr) continue;
		const AnimationStateComponent* animState = object.animationStateComponent.has_value()
			? &object.animationStateComponent.value()
			: nullptr;
		const float is = object.renderComponent->importScale;
		const XMMATRIX model = XMMatrixScaling(is, is, is) * object.transform.GetMatrix();
		TransformData td = {};
		XMStoreFloat4x4(&td.mvp,   XMMatrixTranspose(model * lightViewProj));
		XMStoreFloat4x4(&td.model, XMMatrixTranspose(model));
		transformBuffer.Update(gfx, td);
		const SkinningData skinData = ComputeSkinningData(*skinned, animState);
		skinningBuffer.Update(gfx, skinData);
		skinned->vertexBuffer.Bind(gfx);
		skinned->indexBuffer.Bind(gfx);
		gfx.DrawIndexed(skinned->indexBuffer.GetCount());
	}

	shadowPass.End(gfx);
	gfx.RestoreBackBuffer();
}

void Renderer::Render(const Scene& scene, const TopDownCamera& camera)
{
	using namespace DirectX;

	// Upload light data once per frame (uniform scale assumed, so model matrix
	// upper-left 3x3 is a valid normal matrix without inverse-transpose).
	const XMVECTOR rawDir = XMVector3Normalize(XMVectorSet(0.3f, -1.0f, 0.2f, 0.0f));
	LightData lightData = {};
	XMStoreFloat3(&lightData.lightDir,   rawDir);
	// Tuned for the sRGB-correct output pipeline (Engine B). Pre-sRGB
	// these were ambient=0.35 / lightColor=1.0 to compensate for the
	// display undershoot; now those values produce a sun-bleached
	// overbright look. ambient 0.18 + lightColor ≈0.85 keeps the peak
	// `ambient + nDotL*lightColor` close to 1.0 so directly-lit
	// surfaces sit at "natural daylight" rather than blown-out white,
	// and shadowed faces retain visible color instead of going flat.
	lightData.ambient    = 0.18f;
	lightData.lightColor = { 0.86f, 0.83f, 0.78f };
	lightBuffer.Update(gfx, lightData);

	// Shadow depth pass (Shadow S2): render all casters from the sun's POV
	// into the shadow map, then restore the back buffer for the main pass.
	const XMMATRIX lightViewProj = ComputeLightViewProj(camera, rawDir);
	RenderShadowDepth(scene, lightViewProj);

	// Upload the same light VP for the main pass to project each pixel into
	// shadow-map space, and bind the shadow map (t1) + comparison sampler
	// (s1) for the lit/skinned pixel shaders to sample.
	ShadowData shadowData = {};
	XMStoreFloat4x4(&shadowData.lightViewProj, XMMatrixTranspose(lightViewProj));
	shadowBuffer.Update(gfx, shadowData);
	shadowPass.BindForRead(gfx, 1u, 1u);

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
		const AnimationStateComponent* animState = object.animationStateComponent.has_value()
			? &object.animationStateComponent.value()
			: nullptr;
		DrawSkinnedMesh(*skinned, object.transform, *object.renderComponent, camera, animState);
	}

	// Release the shadow map SRV so next frame's depth pass can bind it as
	// a depth-stencil target again (D3D warns if a resource is bound as
	// both input and output).
	shadowPass.UnbindForRead(gfx, 1u);
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

	// IBMs from cgltf are 16 floats in COLUMN-major order (glTF spec).
	// DirectXMath's XMFLOAT4X4 is row-major in memory, so a raw memcpy of
	// column-major bytes into a row-major struct ALREADY produces the
	// transpose (M^T) — exactly the form we want for our row-vector mul
	// convention (`mul(v, skin)` in HLSL means v*M, with translation in the
	// last *row*). No additional XMMatrixTranspose needed; doing one would
	// flip back to column-vector form and put translations in the wrong
	// place (visible as limb stretching on multi-joint rigs).
	std::vector<XMFLOAT4X4> ibmsRowMajor(imported.jointCount);
	for (std::uint32_t j = 0; j < imported.jointCount; ++j)
	{
		const float* src = imported.inverseBindMatrices.data() + j * 16;
		std::memcpy(&ibmsRowMajor[j], src, sizeof(XMFLOAT4X4));
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

	// Same memcpy-only conversion as IBMs above. cgltf_node_transform_world
	// writes column-major bytes; copying into row-major XMFLOAT4X4 yields
	// M^T which is exactly what our row-vector multiply convention wants.
	// Identity-initialized slots from the importer pass through unchanged
	// (identity is its own transpose).
	std::vector<XMFLOAT4X4> jpbwRowMajor(imported.jointCount);
	for (std::uint32_t j = 0; j < imported.jointCount; ++j)
	{
		const float* src = imported.jointParentBaseWorld.data() + j * 16;
		std::memcpy(&jpbwRowMajor[j], src, sizeof(XMFLOAT4X4));
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
			std::move(jpbwRowMajor),
			std::move(imported.animations)));
	for (const auto& tx : imported.textures)
	{
		std::unique_ptr<Texture2D> tex;
		if (!tx.bytes.empty())
		{
			try
			{
				Surface surf = ImageLoader::LoadSurfaceFromMemory(tx.bytes.data(), tx.bytes.size());
				tex = std::make_unique<Texture2D>(gfx, surf);
			}
			catch (const std::exception&) {}
		}
		it->second.textures.push_back(std::move(tex));
	}
	it->second.submeshes = imported.submeshes;
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

void Renderer::DrawSkinnedMesh(const SkinnedMeshBuffers& mesh, const Transform& transform, const RenderComponent& renderComponent, const TopDownCamera& camera, const AnimationStateComponent* animState) noexcept
{
	using namespace DirectX;

	// Asset-level import scale (see DrawMesh comment). Multiplied in
	// before the transform's TRS so it visually rescales the mesh
	// without disturbing the actor's logical scale.
	const float is = renderComponent.importScale;
	const XMMATRIX modelMatrix = XMMatrixScaling(is, is, is) * transform.GetMatrix();
	const XMMATRIX viewProjection = camera.GetViewMatrix() * camera.GetProjectionMatrix();

	TransformData transformData = {};
	XMStoreFloat4x4(&transformData.mvp,   XMMatrixTranspose(modelMatrix * viewProjection));
	XMStoreFloat4x4(&transformData.model, XMMatrixTranspose(modelMatrix));
	transformBuffer.Update(gfx, transformData);

	const Material mat = renderComponent.material.value_or(Material{});
	const MaterialData materialData = { mat.tint, 0.0f };
	materialBuffer.Update(gfx, materialData);

	const SkinningData skinData = ComputeSkinningData(mesh, animState);
	skinningBuffer.Update(gfx, skinData);

	ResolveSampler(mat.sampler).Bind(gfx);
	mesh.vertexBuffer.Bind(gfx);
	mesh.indexBuffer.Bind(gfx);

	if (mesh.submeshes.empty())
	{
		ResolveTexture(mat.texturePath).Bind(gfx);
		gfx.DrawIndexed(mesh.indexBuffer.GetCount());
	}
	else
	{
		for (const auto& sub : mesh.submeshes)
		{
			Texture2D* tex = nullptr;
			if (sub.textureIndex >= 0 && sub.textureIndex < static_cast<int>(mesh.textures.size()))
			{
				tex = mesh.textures[sub.textureIndex].get();
			}
			if (tex) tex->Bind(gfx);
			else     defaultWhiteTexture.Bind(gfx);
			gfx.GetContext()->DrawIndexed(sub.indexCount, sub.indexOffset, 0);
		}
	}
}

void Renderer::SamplePose(const SkinnedMeshBuffers& mesh, const std::string& clipName, float clipTime, bool looping,
	std::vector<DirectX::XMVECTOR>& outT, std::vector<DirectX::XMVECTOR>& outR, std::vector<DirectX::XMVECTOR>& outS) const noexcept
{
	using namespace DirectX;

	const std::uint32_t jointCount = static_cast<std::uint32_t>(mesh.inverseBindMatrices.size());

	// Pick the clip: named lookup, falling back to clip[0]; nullptr when the
	// mesh has no clips (=> pure bind pose).
	const ImportedAnimation* clip = nullptr;
	if (!mesh.animations.empty())
	{
		if (!clipName.empty())
		{
			for (const auto& a : mesh.animations)
			{
				if (a.name == clipName) { clip = &a; break; }
			}
		}
		if (clip == nullptr) clip = &mesh.animations[0];
	}
	const bool hasClip = (clip != nullptr) && !clip->channels.empty() && clip->duration > 0.0f;
	// Looping clips wrap with fmod; non-looping (Die, one-shots) clamp to
	// the last frame. The blend-from clip is always sampled clamped so its
	// frozen snapshot pose doesn't wrap back to frame 0 mid-fade.
	const float t = hasClip
		? (looping ? std::fmod(clipTime, clip->duration) : std::min(clipTime, clip->duration))
		: 0.0f;

	// Initialize every joint to its bind-pose local TRS; channels override.
	outT.resize(jointCount);
	outR.resize(jointCount);
	outS.resize(jointCount);
	for (std::uint32_t i = 0; i < jointCount; ++i)
	{
		outT[i] = XMLoadFloat3(&mesh.bindTranslation[i]);
		outR[i] = XMLoadFloat4(&mesh.bindRotation[i]);
		outS[i] = XMLoadFloat3(&mesh.bindScale[i]);
	}

	if (!hasClip) return;

	for (const auto& ch : clip->channels)
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
			outT[ch.targetJoint] = useStep ? v0 : XMVectorLerp(v0, v1, alpha);
			break;
		}
		case ImportedAnimationChannel::Path::Scale:
		{
			const XMVECTOR v0 = XMVectorSet(ch.values[k *3+0], ch.values[k *3+1], ch.values[k *3+2], 0.0f);
			const XMVECTOR v1 = XMVectorSet(ch.values[k1*3+0], ch.values[k1*3+1], ch.values[k1*3+2], 0.0f);
			outS[ch.targetJoint] = useStep ? v0 : XMVectorLerp(v0, v1, alpha);
			break;
		}
		case ImportedAnimationChannel::Path::Rotation:
		{
			const XMVECTOR q0 = XMVectorSet(ch.values[k *4+0], ch.values[k *4+1], ch.values[k *4+2], ch.values[k *4+3]);
			const XMVECTOR q1 = XMVectorSet(ch.values[k1*4+0], ch.values[k1*4+1], ch.values[k1*4+2], ch.values[k1*4+3]);
			outR[ch.targetJoint] = useStep ? q0 : XMQuaternionSlerp(q0, q1, alpha);
			break;
		}
		}
	}
}

Renderer::SkinningData Renderer::ComputeSkinningData(const SkinnedMeshBuffers& mesh, const AnimationStateComponent* animState) const noexcept
{
	using namespace DirectX;

	// === Per-frame pose computation ===========================================
	// 1. Sample the active clip into per-joint local TRS (SamplePose).
	// 2. If a crossfade is in flight, sample the outgoing clip too and
	//    lerp(T/S) / slerp(R) the two poses by the fade weight.
	// 3. Compose local matrix = S*R*T, walk the hierarchy to world.
	// 4. jointMatrix[i] = inverseBindMatrix[i] * jointWorld[i], transposed
	//    for HLSL's column-major default.
	const std::uint32_t jointCount = static_cast<std::uint32_t>(mesh.inverseBindMatrices.size());

	const std::string clipName = (animState != nullptr) ? animState->clipName : std::string();
	const float clipTime       = (animState != nullptr) ? animState->clipTime : 0.0f;
	const bool  looping        = (animState != nullptr) ? animState->looping  : true;

	std::vector<XMVECTOR> Tvec, Rvec, Svec;
	SamplePose(mesh, clipName, clipTime, looping, Tvec, Rvec, Svec);

	// Crossfade: blend the previous clip's frozen pose into the active one.
	// weight goes 0 -> 1 as blendRemaining drains, so we end on the target.
	if (animState != nullptr && animState->blendRemaining > 0.0f &&
		animState->blendDuration > 0.0f && !animState->blendFromClip.empty())
	{
		std::vector<XMVECTOR> Tb, Rb, Sb;
		SamplePose(mesh, animState->blendFromClip, animState->blendFromTime, /*looping=*/false, Tb, Rb, Sb);
		const float w = 1.0f - (animState->blendRemaining / animState->blendDuration);
		const std::uint32_t n = std::min<std::uint32_t>(jointCount, static_cast<std::uint32_t>(Tb.size()));
		for (std::uint32_t i = 0; i < n; ++i)
		{
			Tvec[i] = XMVectorLerp(Tb[i], Tvec[i], w);
			Rvec[i] = XMQuaternionSlerp(Rb[i], Rvec[i], w);
			Svec[i] = XMVectorLerp(Sb[i], Svec[i], w);
		}
	}

	// Compose local matrices, then walk hierarchy for world matrices.
	// Root joints (parent < 0) consume their pre-baked parentBaseWorld
	// (the transform of the non-joint ancestor chain — Armature / Z_UP /
	// scene root). Non-root joints chain through the parent's already-
	// computed jointWorld.
	std::vector<XMMATRIX> jointWorld(jointCount);
	for (std::uint32_t i = 0; i < jointCount; ++i)
	{
		const XMMATRIX S = XMMatrixScalingFromVector(Svec[i]);
		const XMMATRIX R = XMMatrixRotationQuaternion(Rvec[i]);
		const XMMATRIX T = XMMatrixTranslationFromVector(Tvec[i]);
		const XMMATRIX local = S * R * T;
		const int parent = (i < mesh.jointParents.size()) ? mesh.jointParents[i] : -1;
		XMMATRIX parentMat;
		if (parent >= 0)
		{
			parentMat = jointWorld[parent];
		}
		else
		{
			parentMat = (i < mesh.jointParentBaseWorld.size())
				? XMLoadFloat4x4(&mesh.jointParentBaseWorld[i])
				: XMMatrixIdentity();
		}
		jointWorld[i] = local * parentMat;
	}

	// Safety: pre-fill all MaxJoints slots with transposed identity. If the
	// asset's JOINTS_0 happens to reference any index in [count, MaxJoints)
	// (malformed export), the offending vertices stay at their bind position
	// instead of collapsing to origin and creating long stretched triangles.
	SkinningData skinData = {};
	XMFLOAT4X4 identityRowMajor;
	XMStoreFloat4x4(&identityRowMajor, XMMatrixTranspose(XMMatrixIdentity()));
	for (std::uint32_t i = 0; i < SkinnedPipeline::MaxJoints; ++i)
	{
		skinData.jointMatrices[i] = identityRowMajor;
	}
	const std::uint32_t count = std::min<std::uint32_t>(jointCount, SkinnedPipeline::MaxJoints);
	for (std::uint32_t i = 0; i < count; ++i)
	{
		const XMMATRIX ibm = XMLoadFloat4x4(&mesh.inverseBindMatrices[i]);
		// Row-vector convention: pos * jointMatrix = pos * IBM * jointWorld.
		// In DirectXMath multiply order this is `IBM * jointWorld`.
		const XMMATRIX jm = ibm * jointWorld[i];
		XMStoreFloat4x4(&skinData.jointMatrices[i], XMMatrixTranspose(jm));
	}
	return skinData;
}
