#pragma once
#include "../../Core/Graphics.h"
#include <wrl/client.h>

class PixelShader
{
public:
	PixelShader(Graphics& gfx, const char* source, const char* entryPoint);
	void Bind(Graphics& gfx) const noexcept;
private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader;
};
