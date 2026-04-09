#pragma once
#include "../../Core/Graphics.h"
#include <wrl/client.h>

class VertexBuffer
{
public:
	VertexBuffer(Graphics& gfx, const void* data, UINT byteWidth, UINT stride);
	void Bind(Graphics& gfx) const noexcept;
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer;
	UINT stride;
};
