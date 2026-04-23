#include "Graphics.h"
#include "Dxerr/dxerr.h"
#include <sstream>
#include <wrl/client.h>
#include <dxgi.h>

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
	using Microsoft::WRL::ComPtr;

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
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		pSwapChain.GetAddressOf(), 
		pDevice.GetAddressOf(), 
		nullptr, 
		pContext.GetAddressOf()
	)))
	{
		throw BFGFX_EXCEPT(hr);
	}

	// gain access to texture subresource in swap chain (back buffer)
	ComPtr<ID3D11Resource> pBackBuffer;
	if (FAILED(hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), reinterpret_cast<void**>(pBackBuffer.GetAddressOf()))))
	{
		throw BFGFX_EXCEPT(hr);
	}

	if (FAILED(hr = pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, pRenderTarget.GetAddressOf())))
	{
		throw BFGFX_EXCEPT(hr);
	}

	RECT rect;
	GetClientRect(hWnd, &rect);
	const UINT width = static_cast<UINT>(rect.right - rect.left);
	const UINT height = static_cast<UINT>(rect.bottom - rect.top);

	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.MipLevels = 1u;
	depthDesc.ArraySize = 1u;
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.SampleDesc.Count = 1u;
	depthDesc.SampleDesc.Quality = 0u;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ComPtr<ID3D11Texture2D> pDepthStencil;
	if (FAILED(hr = pDevice->CreateTexture2D(&depthDesc, nullptr, pDepthStencil.GetAddressOf())))
	{
		throw BFGFX_EXCEPT(hr);
	}

	if (FAILED(hr = pDevice->CreateDepthStencilView(pDepthStencil.Get(), nullptr, pDepthStencilView.GetAddressOf())))
	{
		throw BFGFX_EXCEPT(hr);
	}

	ID3D11RenderTargetView* const renderTargets[] = { pRenderTarget.Get() };
	pContext->OMSetRenderTargets(1u, renderTargets, pDepthStencilView.Get());

	const D3D11_VIEWPORT viewport =
	{
		0.0f,
		0.0f,
		static_cast<float>(width),
		static_cast<float>(height),
		0.0f,
		1.0f
	};
	pContext->RSSetViewports(1u, &viewport);

	// D2D / DWrite factories (required for in-viewport text overlay).
	if (FAILED(hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		__uuidof(ID2D1Factory),
		reinterpret_cast<void**>(pD2DFactory.GetAddressOf()))))
	{
		throw BFGFX_EXCEPT(hr);
	}
	if (FAILED(hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(pDWriteFactory.GetAddressOf()))))
	{
		throw BFGFX_EXCEPT(hr);
	}

	RecreateD2DTarget();
}

Graphics::~Graphics() = default;

void Graphics::BeginFrame(float red, float green, float blue) noexcept
{
	// Defensive re-bind: D2D's EndDraw can clobber OM state. Without this,
	// any frame after a text pass would lose the render target and render black.
	ID3D11RenderTargetView* const renderTargets[] = { pRenderTarget.Get() };
	pContext->OMSetRenderTargets(1u, renderTargets, pDepthStencilView.Get());

	if (d2dTargetNeedsRecreate)
	{
		pD2DTarget.Reset();
		try
		{
			RecreateD2DTarget();
		}
		catch (...)
		{
			// Swallow: text will be skipped until the next successful recreate.
		}
		d2dTargetNeedsRecreate = false;
	}

	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pRenderTarget.Get(), color);
	pContext->ClearDepthStencilView(pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0u);
}

ID3D11Device* Graphics::GetDevice() noexcept
{
	return pDevice.Get();
}

ID3D11DeviceContext* Graphics::GetContext() noexcept
{
	return pContext.Get();
}

void Graphics::DrawIndexed(UINT count) noexcept
{
	pContext->DrawIndexed(count, 0u, 0);
}

void Graphics::EndFrame()
{
	if (const HRESULT hr = pSwapChain->Present(1u, 0u); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}

ID2D1RenderTarget* Graphics::GetD2DTarget() noexcept
{
	return pD2DTarget.Get();
}

IDWriteFactory* Graphics::GetDWriteFactory() noexcept
{
	return pDWriteFactory.Get();
}

void Graphics::BeginTextDraw() noexcept
{
	if (pD2DTarget)
	{
		pD2DTarget->BeginDraw();
	}
}

HRESULT Graphics::EndTextDraw() noexcept
{
	if (!pD2DTarget)
	{
		return S_OK;
	}
	const HRESULT hr = pD2DTarget->EndDraw();
	if (hr == D2DERR_RECREATE_TARGET)
	{
		d2dTargetNeedsRecreate = true;
	}
	return hr;
}

void Graphics::RecreateD2DTarget()
{
	using Microsoft::WRL::ComPtr;

	ComPtr<IDXGISurface> pBackSurface;
	HRESULT hr;
	if (FAILED(hr = pSwapChain->GetBuffer(0u, __uuidof(IDXGISurface), reinterpret_cast<void**>(pBackSurface.GetAddressOf()))))
	{
		throw BFGFX_EXCEPT(hr);
	}

	const D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		0.0f,
		0.0f);

	if (FAILED(hr = pD2DFactory->CreateDxgiSurfaceRenderTarget(
		pBackSurface.Get(),
		&props,
		pD2DTarget.ReleaseAndGetAddressOf())))
	{
		throw BFGFX_EXCEPT(hr);
	}
}
