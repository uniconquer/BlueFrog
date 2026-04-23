#pragma once
#include "BFWin.h"
#include "BFException.h"
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

class Graphics
{
public:
	class Exception : public BFException
	{
	public:
		Exception(int line, const char* file, HRESULT hr) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		static std::string TranslateErrorCode(HRESULT hr) noexcept;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorString() const noexcept;
	private:
		HRESULT hr;
	};
public:
	Graphics(HWND hWnd);
	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;
	~Graphics();
	void BeginFrame(float red, float green, float blue) noexcept;
	void EndFrame();
	void DrawIndexed(UINT count) noexcept;
	ID3D11Device* GetDevice() noexcept;
	ID3D11DeviceContext* GetContext() noexcept;
	ID2D1RenderTarget* GetD2DTarget() noexcept;
	IDWriteFactory* GetDWriteFactory() noexcept;
	void BeginTextDraw() noexcept;
	HRESULT EndTextDraw() noexcept;
private:
	void RecreateD2DTarget();
private:
	Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
	Microsoft::WRL::ComPtr<IDXGISwapChain> pSwapChain;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTarget;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDepthStencilView;
	Microsoft::WRL::ComPtr<ID2D1Factory> pD2DFactory;
	Microsoft::WRL::ComPtr<IDWriteFactory> pDWriteFactory;
	Microsoft::WRL::ComPtr<ID2D1RenderTarget> pD2DTarget;
	bool d2dTargetNeedsRecreate = false;
};

#define BFGFX_EXCEPT(hr) Graphics::Exception(__LINE__, __FILE__, hr)
