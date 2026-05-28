#include "ShadowMapPass.h"

ShadowMapPass::ShadowMapPass(Graphics& gfx)
{
	ID3D11Device* device = gfx.GetDevice();

	// Depth texture as R32_TYPELESS so we can make BOTH a depth-stencil
	// view (D32_FLOAT, written during the depth pass) and a shader
	// resource view (R32_FLOAT, sampled during the main pass) over the
	// same memory.
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = kSize;
	texDesc.Height = kSize;
	texDesc.MipLevels = 1u;
	texDesc.ArraySize = 1u;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texDesc.SampleDesc.Count = 1u;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
	if (const HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0u;
	if (const HRESULT hr = device->CreateDepthStencilView(tex.Get(), &dsvDesc, pDsv.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0u;
	srvDesc.Texture2D.MipLevels = 1u;
	if (const HRESULT hr = device->CreateShaderResourceView(tex.Get(), &srvDesc, pSrv.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}

	// Comparison sampler for hardware PCF: SampleCmp returns a 0..1 value
	// (already bilinearly filtered) of "fraction of the 2x2 tap that the
	// pixel passes the depth test against". LESS_EQUAL because we store
	// depth and want "pixel depth <= stored depth => lit".
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	// Border = 1.0 (max depth) so anything sampled outside the shadow map
	// frustum reads as "fully lit" rather than "fully shadowed".
	sampDesc.BorderColor[0] = 1.0f;
	sampDesc.BorderColor[1] = 1.0f;
	sampDesc.BorderColor[2] = 1.0f;
	sampDesc.BorderColor[3] = 1.0f;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	if (const HRESULT hr = device->CreateSamplerState(&sampDesc, pComparisonSampler.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}

void ShadowMapPass::Begin(Graphics& gfx) noexcept
{
	ID3D11DeviceContext* ctx = gfx.GetContext();

	// Depth-only: bind the DSV with a null render target.
	ID3D11RenderTargetView* nullRtv[] = { nullptr };
	ctx->OMSetRenderTargets(1u, nullRtv, pDsv.Get());

	const D3D11_VIEWPORT vp =
	{
		0.0f, 0.0f,
		static_cast<float>(kSize),
		static_cast<float>(kSize),
		0.0f, 1.0f
	};
	ctx->RSSetViewports(1u, &vp);

	ctx->ClearDepthStencilView(pDsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0u);
}

void ShadowMapPass::End(Graphics& gfx) noexcept
{
	// Unbind the DSV so the texture is free to be read as an SRV. Binding
	// a null target here avoids the "resource is still bound as output"
	// warning when we set it as input in the main pass.
	ID3D11RenderTargetView* nullRtv[] = { nullptr };
	gfx.GetContext()->OMSetRenderTargets(1u, nullRtv, nullptr);
}

void ShadowMapPass::BindForRead(Graphics& gfx, UINT srvSlot, UINT samplerSlot) noexcept
{
	ID3D11DeviceContext* ctx = gfx.GetContext();
	ID3D11ShaderResourceView* srv = pSrv.Get();
	ID3D11SamplerState* samp = pComparisonSampler.Get();
	ctx->PSSetShaderResources(srvSlot, 1u, &srv);
	ctx->PSSetSamplers(samplerSlot, 1u, &samp);
}

void ShadowMapPass::UnbindForRead(Graphics& gfx, UINT srvSlot) noexcept
{
	ID3D11ShaderResourceView* nullSrv[] = { nullptr };
	gfx.GetContext()->PSSetShaderResources(srvSlot, 1u, nullSrv);
}
