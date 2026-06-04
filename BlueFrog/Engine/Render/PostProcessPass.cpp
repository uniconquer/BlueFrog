#include "PostProcessPass.h"

#include <algorithm>

namespace
{
	// Shared full-screen triangle VS (no vertex buffer; SV_VertexID) + the
	// composite/tonemap PS. Composite adds blurred bloom to the scene, applies
	// exposure, then ACES; output is linear [0,1] which the sRGB back-buffer
	// RTV encodes on store.
	const char* kMainSource =
		"struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
		"VSOut VSMain(uint id : SV_VertexID)\n"
		"{\n"
		"    VSOut o;\n"
		"    o.uv  = float2((id << 1) & 2, id & 2);\n"
		"    o.pos = float4(o.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);\n"
		"    return o;\n"
		"}\n"
		"Texture2D sceneTex : register(t0);\n"
		"Texture2D bloomTex : register(t1);\n"
		"SamplerState samp : register(s0);\n"
		"cbuffer Post : register(b0) { float exposure; float bloomThreshold; float bloomIntensity; float saturation; float2 blurDir; float contrast; float pad2; };\n"
		"float3 ACESFilm(float3 x)\n"
		"{\n"
		"    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;\n"
		"    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));\n"
		"}\n"
		"float4 PSMain(VSOut i) : SV_Target\n"
		"{\n"
		"    float3 scene = sceneTex.Sample(samp, i.uv).rgb;\n"
		"    float3 bloom = bloomTex.Sample(samp, i.uv).rgb;\n"
		"    float3 hdr   = (scene + bloom * bloomIntensity) * exposure;\n"
		"    float3 col   = ACESFilm(hdr);\n"
		// Painterly grade: lift saturation (vibrance) + a gentle S-curve
		// contrast so the stylized kit reads punchy like the kit's promo art
		// instead of washed-out flat.
		"    float luma   = dot(col, float3(0.2126, 0.7152, 0.0722));\n"
		"    col          = lerp(float3(luma, luma, luma), col, saturation);\n"
		"    col          = saturate((col - 0.5) * contrast + 0.5);\n"
		"    return float4(col, 1.0);\n"
		"}\n";

	// Bright-pass: keep only the HDR energy above the threshold.
	const char* kBrightSource =
		"struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
		"Texture2D sceneTex : register(t0);\n"
		"SamplerState samp : register(s0);\n"
		"cbuffer Post : register(b0) { float exposure; float bloomThreshold; float bloomIntensity; float pad0; float2 blurDir; float2 pad1; };\n"
		"float4 PSMain(VSOut i) : SV_Target\n"
		"{\n"
		"    float3 c = sceneTex.Sample(samp, i.uv).rgb;\n"
		"    return float4(max(c - bloomThreshold, 0.0), 1.0);\n"
		"}\n";

	// Separable 9-tap Gaussian; blurDir is one texel step along H or V (UV).
	const char* kBlurSource =
		"struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
		"Texture2D srcTex : register(t0);\n"
		"SamplerState samp : register(s0);\n"
		"cbuffer Post : register(b0) { float exposure; float bloomThreshold; float bloomIntensity; float pad0; float2 blurDir; float2 pad1; };\n"
		"float4 PSMain(VSOut i) : SV_Target\n"
		"{\n"
		"    float w[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };\n"
		"    float3 sum = srcTex.Sample(samp, i.uv).rgb * w[0];\n"
		"    [unroll] for (int k = 1; k < 5; ++k)\n"
		"    {\n"
		"        sum += srcTex.Sample(samp, i.uv + blurDir * k).rgb * w[k];\n"
		"        sum += srcTex.Sample(samp, i.uv - blurDir * k).rgb * w[k];\n"
		"    }\n"
		"    return float4(sum, 1.0);\n"
		"}\n";

	Microsoft::WRL::ComPtr<ID3D11Texture2D> MakeHdrTex(ID3D11Device* device, UINT w, UINT h,
		ID3D11RenderTargetView** rtv, ID3D11ShaderResourceView** srv)
	{
		D3D11_TEXTURE2D_DESC d = {};
		d.Width = w; d.Height = h; d.MipLevels = 1u; d.ArraySize = 1u;
		d.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d.SampleDesc.Count = 1u;
		d.Usage = D3D11_USAGE_DEFAULT;
		d.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
		if (FAILED(device->CreateTexture2D(&d, nullptr, tex.GetAddressOf()))) throw BFGFX_EXCEPT(E_FAIL);
		if (FAILED(device->CreateRenderTargetView(tex.Get(), nullptr, rtv))) throw BFGFX_EXCEPT(E_FAIL);
		if (FAILED(device->CreateShaderResourceView(tex.Get(), nullptr, srv))) throw BFGFX_EXCEPT(E_FAIL);
		return tex;
	}
}

PostProcessPass::PostProcessPass(Graphics& gfx)
	: width(gfx.GetBackBufferWidth())
	, height(gfx.GetBackBufferHeight())
	, bloomW(std::max<UINT>(1u, gfx.GetBackBufferWidth() / 2u))
	, bloomH(std::max<UINT>(1u, gfx.GetBackBufferHeight() / 2u))
	, fullscreenVS(gfx, kMainSource, "VSMain")
	, tonemapPS(gfx, kMainSource, "PSMain")
	, brightPassPS(gfx, kBrightSource, "PSMain")
	, blurPS(gfx, kBlurSource, "PSMain")
	, sampler(gfx, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP)
	, paramsBuffer(gfx)
{
	ID3D11Device* device = gfx.GetDevice();

	pHdrTexture = MakeHdrTex(device, width, height, pHdrRtv.GetAddressOf(), pHdrSrv.GetAddressOf());
	pBloomTex[0] = MakeHdrTex(device, bloomW, bloomH, pBloomRtv[0].GetAddressOf(), pBloomSrv[0].GetAddressOf());
	pBloomTex[1] = MakeHdrTex(device, bloomW, bloomH, pBloomRtv[1].GetAddressOf(), pBloomSrv[1].GetAddressOf());

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

void PostProcessPass::SetTarget(Graphics& gfx, ID3D11RenderTargetView* rtv, UINT w, UINT h) noexcept
{
	ID3D11DeviceContext* ctx = gfx.GetContext();
	ctx->OMSetRenderTargets(1u, &rtv, nullptr);
	const D3D11_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f };
	ctx->RSSetViewports(1u, &vp);
}

void PostProcessPass::Resolve(Graphics& gfx, float exposure, float saturation, float contrast) noexcept
{
	ID3D11DeviceContext* ctx = gfx.GetContext();

	ctx->OMSetDepthStencilState(pNoDepthState.Get(), 0u);
	ctx->IASetInputLayout(nullptr);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	fullscreenVS.Bind(gfx);
	sampler.Bind(gfx);

	auto clearSRVs = [&]()
	{
		ID3D11ShaderResourceView* nulls[2] = { nullptr, nullptr };
		ctx->PSSetShaderResources(0u, 2u, nulls);
	};
	auto bind0 = [&](ID3D11ShaderResourceView* s)
	{
		ctx->PSSetShaderResources(0u, 1u, &s);
	};

	PostParams p;
	p.exposure   = exposure;
	p.saturation = saturation;
	p.contrast   = contrast;

	// 1) Bright-pass: full-res HDR -> half-res bloom[0].
	clearSRVs();
	SetTarget(gfx, pBloomRtv[0].Get(), bloomW, bloomH);
	paramsBuffer.Update(gfx, p);
	paramsBuffer.Bind(gfx, 0u);
	brightPassPS.Bind(gfx);
	bind0(pHdrSrv.Get());
	ctx->Draw(3u, 0u);

	// 2) Blur horizontal: bloom[0] -> bloom[1].
	clearSRVs();
	SetTarget(gfx, pBloomRtv[1].Get(), bloomW, bloomH);
	p.blurDirX = 1.0f / static_cast<float>(bloomW); p.blurDirY = 0.0f;
	paramsBuffer.Update(gfx, p);
	blurPS.Bind(gfx);
	bind0(pBloomSrv[0].Get());
	ctx->Draw(3u, 0u);

	// 3) Blur vertical: bloom[1] -> bloom[0].
	clearSRVs();
	SetTarget(gfx, pBloomRtv[0].Get(), bloomW, bloomH);
	p.blurDirX = 0.0f; p.blurDirY = 1.0f / static_cast<float>(bloomH);
	paramsBuffer.Update(gfx, p);
	bind0(pBloomSrv[1].Get());
	ctx->Draw(3u, 0u);

	// 4) Composite to back buffer: scene + bloom, exposure, ACES.
	clearSRVs();
	gfx.RestoreBackBuffer();              // back buffer + depth + full viewport
	ctx->OMSetDepthStencilState(pNoDepthState.Get(), 0u);
	p.blurDirX = 0.0f; p.blurDirY = 0.0f;
	paramsBuffer.Update(gfx, p);
	tonemapPS.Bind(gfx);
	ID3D11ShaderResourceView* srvs[2] = { pHdrSrv.Get(), pBloomSrv[0].Get() };
	ctx->PSSetShaderResources(0u, 2u, srvs);
	ctx->Draw(3u, 0u);

	clearSRVs();
}
