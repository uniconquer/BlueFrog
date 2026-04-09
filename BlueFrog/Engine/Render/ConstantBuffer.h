#pragma once
#include "../../Core/Graphics.h"
#include <wrl/client.h>

template<class C>
class ConstantBuffer
{
public:
	explicit ConstantBuffer(Graphics& gfx)
	{
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.CPUAccessFlags = 0u;
		bufferDesc.MiscFlags = 0u;
		bufferDesc.ByteWidth = sizeof(C);
		bufferDesc.StructureByteStride = 0u;

		if (const HRESULT hr = gfx.GetDevice()->CreateBuffer(&bufferDesc, nullptr, pConstantBuffer.GetAddressOf()); FAILED(hr))
		{
			throw BFGFX_EXCEPT(hr);
		}
	}

	void Update(Graphics& gfx, const C& data) noexcept
	{
		gfx.GetContext()->UpdateSubresource(pConstantBuffer.Get(), 0u, nullptr, &data, 0u, 0u);
	}

	ID3D11Buffer* Get() const noexcept
	{
		return pConstantBuffer.Get();
	}
protected:
	Microsoft::WRL::ComPtr<ID3D11Buffer> pConstantBuffer;
};

template<class C>
class VertexConstantBuffer : public ConstantBuffer<C>
{
public:
	using ConstantBuffer<C>::ConstantBuffer;

	void Bind(Graphics& gfx, UINT slot = 0u) const noexcept
	{
		ID3D11Buffer* const buffer = this->pConstantBuffer.Get();
		gfx.GetContext()->VSSetConstantBuffers(slot, 1u, &buffer);
	}
};

template<class C>
class PixelConstantBuffer : public ConstantBuffer<C>
{
public:
	using ConstantBuffer<C>::ConstantBuffer;

	void Bind(Graphics& gfx, UINT slot = 0u) const noexcept
	{
		ID3D11Buffer* const buffer = this->pConstantBuffer.Get();
		gfx.GetContext()->PSSetConstantBuffers(slot, 1u, &buffer);
	}
};
