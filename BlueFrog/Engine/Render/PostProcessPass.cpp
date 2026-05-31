#include "PostProcessPass.h"

namespace
{
	// Full-screen triangle (no vertex buffer; positions from SV_VertexID) +
	// exposure/ACES tonemap. Input is the linear HDR scene; output is linear
	// [0,1] which the sRGB back-buffer RTV encodes to sRGB on store.
	const char* kPostShaderSource =
		"struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
		"VSOut VSMain(uint id : SV_VertexID)\n"
		"{\n"
		"    VSOut o;\n"
		"    o.uv  = float2((id << 1) & 2, id & 2);\n"
		"    o.pos = float4(o.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);\n"
		"    return o;\n"
		"}\n"
		"Texture2D hdrTex : register(t0);\n"
		"SamplerState samp : register(s0);\n"
		"cbuffer Post : register(b0) { float exposure; float3 pad; };\n"
		// Narkowicz ACES filmic approximation — cheap, no LUT.
		"float3 ACESFilm(float3 x)\n"
		"{\n"
		"    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;\n"
		"    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));\n"
		"}\n"
		"float4 PSMain(VSOut i) : SV_Target\n"
		"{\n"
		"    float3 hdr = hdrTex.Sample(samp, i.uv).rgb * exposure;\n"
		"    return float4(ACESFilm(hdr), 1.0);\n"
		"}\n";
}

PostProcessPass::PostProcessPass(Graphics& gfx)
	: width(gfx.GetBackBufferWidth())
	, height(gfx.GetBackBufferHeight())
	, fullscreenVS(gfx, kPostShaderSource, "VSMain")
	, tonemapPS(gfx, kPostShaderSource, "PSMain")
	, sampler(gfx, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP)
	, paramsBuffer(gfx)
{
	ID3D11Device* device = gfx.GetDevice();

	// Linear HDR scene target (float so lit values can exceed 1.0 before
	// tonemapping; also the source for future bloom).
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1u;
	texDesc.ArraySize = 1u;
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1u;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	if (const HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, pHdrTexture.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
	if (const HRESULT hr = device->CreateRenderTargetView(pHdrTexture.Get(), nullptr, pHdrRtv.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
	if (const HRESULT hr = device->CreateShaderResourceView(pHdrTexture.Get(), nullptr, pHdrSrv.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}

	// Depth disabled for the full-screen resolve so the triangle neither
	// tests nor writes depth.
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = FALSE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	if (const HRESULT hr = device->CreateDepthStencilState(&dsDesc, pNoDepthState.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}

void PostProcessPass::BeginScene(Graphics& gfx, float r, float g, float b) noexcept
{
	ID3D11DeviceContext* ctx = gfx.GetContext();
	ID3D11RenderTargetView* rtv = pHdrRtv.Get();
	ID3D11DepthStencilView* dsv = gfx.GetDepthStencilView();
	ctx->OMSetRenderTargets(1u, &rtv, dsv);
	const D3D11_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
	ctx->RSSetViewports(1u, &vp);
	const float clear[4] = { r, g, b, 1.0f };
	ctx->ClearRenderTargetView(pHdrRtv.Get(), clear);
	ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0u);
}

void PostProcessPass::Resolve(Graphics& gfx, float exposure) noexcept
{
	ID3D11DeviceContext* ctx = gfx.GetContext();

	// Back to the swap-chain RTV + main depth (RestoreBackBuffer also resets
	// the viewport). This unbinds the HDR target as RTV so it can be read.
	gfx.RestoreBackBuffer();
	ctx->OMSetDepthStencilState(pNoDepthState.Get(), 0u);

	PostParams p;
	p.exposure = exposure;
	paramsBuffer.Update(gfx, p);
	paramsBuffer.Bind(gfx, 0u);

	fullscreenVS.Bind(gfx);
	tonemapPS.Bind(gfx);
	sampler.Bind(gfx);
	ID3D11ShaderResourceView* srv = pHdrSrv.Get();
	ctx->PSSetShaderResources(0u, 1u, &srv);

	ctx->IASetInputLayout(nullptr);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ctx->Draw(3u, 0u);

	// Unbind the HDR SRV so next frame's BeginScene can bind it as RTV
	// without a "still bound as input" hazard warning.
	ID3D11ShaderResourceView* nullSrv = nullptr;
	ctx->PSSetShaderResources(0u, 1u, &nullSrv);
}
