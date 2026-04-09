#include "PixelShader.h"
#include <d3dcompiler.h>
#include <cstring>

PixelShader::PixelShader(Graphics& gfx, const char* source, const char* entryPoint)
{
	Microsoft::WRL::ComPtr<ID3DBlob> pShaderBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob;
	if (const HRESULT hr = D3DCompile(
		source,
		strlen(source),
		nullptr,
		nullptr,
		nullptr,
		entryPoint,
		"ps_5_0",
		0u,
		0u,
		pShaderBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf()); FAILED(hr))
	{
		if (pErrorBlob != nullptr)
		{
			OutputDebugStringA(static_cast<const char*>(pErrorBlob->GetBufferPointer()));
		}
		throw BFGFX_EXCEPT(hr);
	}

	if (const HRESULT hr = gfx.GetDevice()->CreatePixelShader(
		pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(),
		nullptr,
		pPixelShader.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}

void PixelShader::Bind(Graphics& gfx) const noexcept
{
	gfx.GetContext()->PSSetShader(pPixelShader.Get(), nullptr, 0u);
}
