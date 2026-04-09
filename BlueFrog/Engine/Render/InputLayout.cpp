#include "InputLayout.h"

InputLayout::InputLayout(Graphics& gfx, const D3D11_INPUT_ELEMENT_DESC* layout, UINT numElements, const VertexShader& vertexShader)
{
	if (const HRESULT hr = gfx.GetDevice()->CreateInputLayout(
		layout,
		numElements,
		vertexShader.GetBytecode()->GetBufferPointer(),
		vertexShader.GetBytecode()->GetBufferSize(),
		pInputLayout.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}

void InputLayout::Bind(Graphics& gfx) const noexcept
{
	gfx.GetContext()->IASetInputLayout(pInputLayout.Get());
}
