#pragma once
#include "../../Core/Graphics.h"
#include <d3dcompiler.h>
#include <wrl/client.h>

class VertexShader
{
public:
	VertexShader(Graphics& gfx, const char* source, const char* entryPoint);
	void Bind(Graphics& gfx) const noexcept;
	ID3DBlob* GetBytecode() const noexcept;
private:
	Microsoft::WRL::ComPtr<ID3DBlob> pBytecodeBlob;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> pVertexShader;
};
