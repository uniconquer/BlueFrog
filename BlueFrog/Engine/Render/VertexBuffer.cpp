#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(Graphics& gfx, const void* data, UINT byteWidth, UINT stride)
	:
	stride(stride)
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.CPUAccessFlags = 0u;
	bufferDesc.MiscFlags = 0u;
	bufferDesc.ByteWidth = byteWidth;
	bufferDesc.StructureByteStride = stride;

	D3D11_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pSysMem = data;

	if (const HRESULT hr = gfx.GetDevice()->CreateBuffer(&bufferDesc, &subresourceData, pVertexBuffer.GetAddressOf()); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}

void VertexBuffer::Bind(Graphics& gfx) const noexcept
{
	const UINT offset = 0u;
	ID3D11Buffer* const buffer = pVertexBuffer.Get();
	gfx.GetContext()->IASetVertexBuffers(0u, 1u, &buffer, &stride, &offset);
}
