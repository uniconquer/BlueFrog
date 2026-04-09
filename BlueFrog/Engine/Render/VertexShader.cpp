#include "VertexShader.h"
#include <cstring>

VertexShader::VertexShader(Graphics& gfx, const char* source, const char* entryPoint)
{
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob;
	if (const HRESULT hr = D3DCompile(
		source,
		strlen(source),
		nullptr,
		nullptr,
		nullptr,
		entryPoint,
		"vs_5_0",
		0u,
		0u,
		pBytecodeBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf()); FAILED(hr))
	{
		if (pErrorBlob != nullptr)
		{
			OutputDebugStringA(static_cast<const char*>(pErrorBlob->GetBufferPointer()));
		}
		throw BFGFX_EXCEPT(hr);
	}

	if (const HRESULT hr = gfx.GetDevice()->CreateVertexShader(
		pBytecodeBlob->GetBufferPointer(),
		pBytecodeBlob->GetBufferSize(),
		nullptr,
		pVertexShader.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}

void VertexShader::Bind(Graphics& gfx) const noexcept
{
	gfx.GetContext()->VSSetShader(pVertexShader.Get(), nullptr, 0u);
}

ID3DBlob* VertexShader::GetBytecode() const noexcept
{
	return pBytecodeBlob.Get();
}
