#pragma once
#include "Graphics.h"

class Renderer
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
	explicit Renderer(Graphics& gfx);
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	~Renderer();
	void DrawTestTriangle(float angle) noexcept;
private:
	Graphics& gfx;
	ID3D11VertexShader* pVertexShader = nullptr;
	ID3D11PixelShader* pPixelShader = nullptr;
	ID3D11InputLayout* pInputLayout = nullptr;
	ID3D11Buffer* pVertexBuffer = nullptr;
	ID3D11Buffer* pTransformBuffer = nullptr;
};

#define BFRENDER_EXCEPT(hr) Renderer::Exception(__LINE__, __FILE__, hr)
