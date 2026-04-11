#pragma once

#include "../../Core/Graphics.h"
#include <wrl/client.h>

class Sampler final
{
public:
	explicit Sampler(
		Graphics& gfx,
		D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_WRAP)
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = filter;
		samplerDesc.AddressU = addressMode;
		samplerDesc.AddressV = addressMode;
		samplerDesc.AddressW = addressMode;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1u;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.BorderColor[0] = 0.0f;
		samplerDesc.BorderColor[1] = 0.0f;
		samplerDesc.BorderColor[2] = 0.0f;
		samplerDesc.BorderColor[3] = 0.0f;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		if (const HRESULT hr = gfx.GetDevice()->CreateSamplerState(&samplerDesc, pSampler.GetAddressOf()); FAILED(hr))
		{
			throw BFGFX_EXCEPT(hr);
		}
	}

	void Bind(Graphics& gfx, UINT slot = 0u) const noexcept
	{
		ID3D11SamplerState* const sampler = pSampler.Get();
		gfx.GetContext()->PSSetSamplers(slot, 1u, &sampler);
	}

private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> pSampler;
};
