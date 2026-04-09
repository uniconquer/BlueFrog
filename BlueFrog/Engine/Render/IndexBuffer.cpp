#include "IndexBuffer.h"

IndexBuffer::IndexBuffer(Graphics& gfx, const unsigned short* indices, UINT count)
	:
	count(count)
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.CPUAccessFlags = 0u;
	bufferDesc.MiscFlags = 0u;
	bufferDesc.ByteWidth = count * static_cast<UINT>(sizeof(unsigned short));
	bufferDesc.StructureByteStride = sizeof(unsigned short);

	D3D11_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pSysMem = indices;

	if (const HRESULT hr = gfx.GetDevice()->CreateBuffer(&bufferDesc, &subresourceData, pIndexBuffer.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}

void IndexBuffer::Bind(Graphics& gfx) const noexcept
{
	gfx.GetContext()->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0u);
}

UINT IndexBuffer::GetCount() const noexcept
{
	return count;
}
