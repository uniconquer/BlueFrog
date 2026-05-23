#pragma once

#include "../../Core/Graphics.h"
#include "Surface.h"
#include <wrl/client.h>

class Texture2D final
{
public:
	explicit Texture2D(Graphics& gfx, const Surface& surface)
	{
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = surface.GetWidth();
		textureDesc.Height = surface.GetHeight();
		textureDesc.MipLevels = 1u;
		textureDesc.ArraySize = 1u;
		// sRGB texture format so hardware auto-decodes sRGB-encoded
		// source PNGs to linear when sampled in the shader. Combined
		// with the _SRGB RTV in Graphics.cpp this completes the
		// linear-shading pipeline: source PNG → linear sample →
		// linear math → sRGB encode on present.
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		textureDesc.SampleDesc.Count = 1u;
		textureDesc.SampleDesc.Quality = 0u;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0u;
		textureDesc.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pSysMem = surface.GetData();
		subresourceData.SysMemPitch = surface.GetPitch();
		subresourceData.SysMemSlicePitch = 0u;

		if (const HRESULT hr = gfx.GetDevice()->CreateTexture2D(&textureDesc, &subresourceData, pTexture.GetAddressOf()); FAILED(hr))
		{
			throw BFGFX_EXCEPT(hr);
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0u;
		srvDesc.Texture2D.MipLevels = 1u;

		if (const HRESULT hr = gfx.GetDevice()->CreateShaderResourceView(pTexture.Get(), &srvDesc, pShaderResourceView.GetAddressOf()); FAILED(hr))
		{
			throw BFGFX_EXCEPT(hr);
		}
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
