#include "Graphics.h"
#include "Dxerr/dxerr.h"
#include <sstream>

Graphics::Exception::Exception(int line, const char* file, HRESULT hr) noexcept
	:
	BFException(line, file),
	hr(hr)
{
}

const char* Graphics::Exception::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode() << std::dec << std::endl
		<< "[Description] " << GetErrorString() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* Graphics::Exception::GetType() const noexcept
{
	return "BlueFrog Graphics Exception";
}

std::string Graphics::Exception::TranslateErrorCode(HRESULT hr) noexcept
{
	char description[512] = {};
	DXGetErrorDescriptionA(hr, description, sizeof(description));

	std::ostringstream oss;
	oss << DXGetErrorStringA(hr);
	if (description[0] != '\0')
	{
		oss << ": " << description;
	}
	return oss.str();
}

HRESULT Graphics::Exception::GetErrorCode() const noexcept
{
	return hr;
}

std::string Graphics::Exception::GetErrorString() const noexcept
{
	return TranslateErrorCode(hr);
}

Graphics::Graphics(HWND hWnd)
{
	HRESULT hr;
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = 0;
	swapChainDesc.BufferDesc.Height = 0;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	// Create device and front/back buffers, and swap chain rendering context
	if (FAILED(hr = D3D11CreateDeviceAndSwapChain(
		nullptr, 
		D3D_DRIVER_TYPE_HARDWARE, 
		nullptr, 
		0, 
		nullptr, 
		0, 
		D3D11_SDK_VERSION, 
		&swapChainDesc,
		&pSwapChain, 
		&pDevice, 
		nullptr, 
		&pContext
	)))
	{
		throw BFGFX_EXCEPT(hr);
	}

	// gain access to texture subresource in swap chain (back buffer)
	ID3D11Resource* pBackBuffer = nullptr;
	if (FAILED(hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), reinterpret_cast<void**>(&pBackBuffer))))
	{
		throw BFGFX_EXCEPT(hr);
	}

	if (FAILED(hr = pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTarget)))
	{
		pBackBuffer->Release();
		throw BFGFX_EXCEPT(hr);
	}
	pBackBuffer->Release();

	pContext->OMSetRenderTargets(1u, &pRenderTarget, nullptr);

	RECT rect;
	GetClientRect(hWnd, &rect);
	const D3D11_VIEWPORT viewport =
	{
		0.0f,
		0.0f,
		static_cast<float>(rect.right - rect.left),
		static_cast<float>(rect.bottom - rect.top),
		0.0f,
		1.0f
	};
	pContext->RSSetViewports(1u, &viewport);
}

Graphics::~Graphics()
{
	if (pRenderTarget != nullptr)
	{
		pRenderTarget->Release();
	}
	if (pContext != nullptr)
	{
		pContext->Release();
	}
	if (pSwapChain != nullptr)
	{
		pSwapChain->Release();
	}
	if (pDevice != nullptr)
	{
		pDevice->Release();
	}
}

void Graphics::EndFrame()
{
	if (const HRESULT hr = pSwapChain->Present(1u, 0u); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}
