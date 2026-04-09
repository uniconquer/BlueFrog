#pragma once
#include "BFWin.h"
#include "BFException.h"
#include <d3d11.h>

class Graphics
{
private:
	struct Vertex
	{
		float x;
		float y;
		float z;
		float r;
		float g;
		float b;
	};

	struct TransformData
	{
		float angle;
		float padding[3];
	};
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
	void DrawTestTriangle(float angle) noexcept;
	void EndFrame();
private:
	ID3D11Device* pDevice = nullptr;
	IDXGISwapChain* pSwapChain = nullptr;
	ID3D11DeviceContext* pContext = nullptr;
	ID3D11RenderTargetView* pRenderTarget = nullptr;
	ID3D11VertexShader* pVertexShader = nullptr;
	ID3D11PixelShader* pPixelShader = nullptr;
	ID3D11InputLayout* pInputLayout = nullptr;
	ID3D11Buffer* pVertexBuffer = nullptr;
	ID3D11Buffer* pTransformBuffer = nullptr;
};

#define BFGFX_EXCEPT(hr) Graphics::Exception(__LINE__, __FILE__, hr)
