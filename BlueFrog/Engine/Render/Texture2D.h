#pragma once

#include "../../Core/Graphics.h"
#include "Surface.h"
#include <wrl/client.h>

class Texture2D final
{
public:
	// srgb=true (default) decodes the source as sRGB -> linear on sample,
	// correct for baseColor/emissive maps. srgb=false keeps bytes raw
	// (UNORM), required for data maps -- metallic-roughness, normal, AO --
	// whose channels are linear values, not perceptual color.
	explicit Texture2D(Graphics& gfx, const Surface& surface, bool srgb = true)
	{
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = surface.GetWidth();
		textureDesc.Height = surface.GetHeight();
		// MipLevels = 0 => full mip chain. Without mips, a high-res texture
		// (2048 kit albedo) viewed minified/tiled aliases down to a flat
		// average color — which is exactly the "textures show only a flat
		// color" bug. We create the chain and GenerateMips below.
		textureDesc.MipLevels = 0u;
		textureDesc.ArraySize = 1u;
		// sRGB format so hardware auto-decodes sRGB-encoded source PNGs to
		// linear when sampled. Combined with the _SRGB RTV in Graphics.cpp
		// this completes the linear-shading pipeline. Data maps pass
		// srgb=false to stay UNORM (no decode).
		textureDesc.Format = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
		                          : DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.SampleDesc.Count = 1u;
		textureDesc.SampleDesc.Quality = 0u;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		// RENDER_TARGET + GENERATE_MIPS are required by ID3D11DeviceContext::
		// GenerateMips. The texture is created empty (no initial data) then
		// mip 0 is uploaded and the rest generated on the GPU.
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		textureDesc.CPUAccessFlags = 0u;
		textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		if (const HRESULT hr = gfx.GetDevice()->CreateTexture2D(&textureDesc, nullptr, pTexture.GetAddressOf()); FAILED(hr))
		{
			throw BFGFX_EXCEPT(hr);
		}

		// Upload the full-res image into mip 0, then let the GPU build the
		// downsampled chain.
		gfx.GetContext()->UpdateSubresource(pTexture.Get(), 0u, nullptr,
			surface.GetData(), surface.GetPitch(), 0u);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0u;
		srvDesc.Texture2D.MipLevels = static_cast<UINT>(-1); // all mip levels

		if (const HRESULT hr = gfx.GetDevice()->CreateShaderResourceView(pTexture.Get(), &srvDesc, pShaderResourceView.GetAddressOf()); FAILED(hr))
		{
			throw BFGFX_EXCEPT(hr);
		}

		gfx.GetContext()->GenerateMips(pShaderResourceView.Get());
	}

	void Bind(Graphics& gfx, UINT slot = 0u) const noexcept
	{
		ID3D11ShaderResourceView* const view = pShaderResourceView.Get();
		gfx.GetContext()->PSSetShaderResources(slot, 1u, &view);
	}

	ID3D11ShaderResourceView* Get() const noexcept
	{
		return pShaderResourceView.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pShaderResourceView;
};
