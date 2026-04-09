#pragma once
#include "../../Core/Graphics.h"
#include "VertexShader.h"
#include <wrl/client.h>

class InputLayout
{
public:
	InputLayout(Graphics& gfx, const D3D11_INPUT_ELEMENT_DESC* layout, UINT numElements, const VertexShader& vertexShader);
	void Bind(Graphics& gfx) const noexcept;
private:
	Microsoft::WRL::ComPtr<ID3D11InputLayout> pInputLayout;
};
