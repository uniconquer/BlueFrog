#pragma once
#include "../../Core/Graphics.h"
#include <wrl/client.h>

class IndexBuffer
{
public:
	IndexBuffer(Graphics& gfx, const unsigned short* indices, UINT count);
	void Bind(Graphics& gfx) const noexcept;
	UINT GetCount() const noexcept;
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> pIndexBuffer;
	UINT count;
};
