#include "Graphics.h"
#include "Dxerr/dxerr.h"
#include <d3dcompiler.h>
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

namespace
{
	template<class T>
	void ReleaseCom(T*& pCom) noexcept
	{
		if (pCom != nullptr)
		{
			pCom->Release();
			pCom = nullptr;
		}
	}
}

Graphics::Graphics(HWND hWnd)
{
	HRESULT hr;
	ID3DBlob* pVertexShaderBlob = nullptr;
	ID3DBlob* pPixelShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	const auto fail = [&](HRESULT failedHr)
	{
		ReleaseCom(pErrorBlob);
		ReleaseCom(pPixelShaderBlob);
		ReleaseCom(pVertexShaderBlob);
		ReleaseCom(pTransformBuffer);
		ReleaseCom(pVertexBuffer);
		ReleaseCom(pInputLayout);
		ReleaseCom(pPixelShader);
		ReleaseCom(pVertexShader);
		ReleaseCom(pRenderTarget);
		ReleaseCom(pContext);
		ReleaseCom(pSwapChain);
		ReleaseCom(pDevice);
		throw BFGFX_EXCEPT(failedHr);
	};
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
		fail(hr);
	}

	// gain access to texture subresource in swap chain (back buffer)
	ID3D11Resource* pBackBuffer = nullptr;
	if (FAILED(hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), reinterpret_cast<void**>(&pBackBuffer))))
	{
		fail(hr);
	}

	if (FAILED(hr = pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTarget)))
	{
		pBackBuffer->Release();
		fail(hr);
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

	const char shaderSource[] =
		"cbuffer TransformBuffer : register(b0)\n"
		"{\n"
		"    float angle;\n"
		"    float3 padding;\n"
		"};\n"
		"struct VSIn\n"
		"{\n"
		"    float3 pos : POSITION;\n"
		"    float3 color : COLOR;\n"
		"};\n"
		"struct PSIn\n"
		"{\n"
		"    float4 pos : SV_Position;\n"
		"    float3 color : COLOR;\n"
		"};\n"
		"PSIn VSMain(VSIn input)\n"
		"{\n"
		"    PSIn output;\n"
		"    const float s = sin(angle);\n"
		"    const float c = cos(angle);\n"
		"    output.pos = float4(input.pos.x * c - input.pos.y * s, input.pos.x * s + input.pos.y * c, input.pos.z, 1.0f);\n"
		"    output.color = input.color;\n"
		"    return output;\n"
		"}\n"
		"float4 PSMain(PSIn input) : SV_Target\n"
		"{\n"
		"    return float4(input.color, 1.0f);\n"
		"}\n";

	if (FAILED(hr = D3DCompile(
		shaderSource, sizeof(shaderSource) - 1u,
		nullptr, nullptr, nullptr,
		"VSMain", "vs_4_0",
		0u, 0u,
		&pVertexShaderBlob, &pErrorBlob)))
	{
		fail(hr);
	}
	ReleaseCom(pErrorBlob);

	if (FAILED(hr = D3DCompile(
		shaderSource, sizeof(shaderSource) - 1u,
		nullptr, nullptr, nullptr,
		"PSMain", "ps_4_0",
		0u, 0u,
		&pPixelShaderBlob, &pErrorBlob)))
	{
		fail(hr);
	}
	ReleaseCom(pErrorBlob);

	if (FAILED(hr = pDevice->CreateVertexShader(
		pVertexShaderBlob->GetBufferPointer(),
		pVertexShaderBlob->GetBufferSize(),
		nullptr,
		&pVertexShader)))
	{
		fail(hr);
	}

	if (FAILED(hr = pDevice->CreatePixelShader(
		pPixelShaderBlob->GetBufferPointer(),
		pPixelShaderBlob->GetBufferSize(),
		nullptr,
		&pPixelShader)))
	{
		fail(hr);
	}

	const D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
		{ "COLOR", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u },
	};

	if (FAILED(hr = pDevice->CreateInputLayout(
		inputElementDesc,
		static_cast<UINT>(std::size(inputElementDesc)),
		pVertexShaderBlob->GetBufferPointer(),
		pVertexShaderBlob->GetBufferSize(),
		&pInputLayout)))
	{
		fail(hr);
	}

	const Vertex vertices[] =
	{
		{ 0.0f, 0.55f, 0.0f, 0.95f, 0.35f, 0.20f },
		{ 0.55f, -0.45f, 0.0f, 0.15f, 0.85f, 0.45f },
		{ -0.55f, -0.45f, 0.0f, 0.20f, 0.50f, 0.95f },
	};

	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.CPUAccessFlags = 0u;
	vertexBufferDesc.MiscFlags = 0u;
	vertexBufferDesc.ByteWidth = sizeof(vertices);
	vertexBufferDesc.StructureByteStride = sizeof(Vertex);

	D3D11_SUBRESOURCE_DATA vertexData = {};
	vertexData.pSysMem = vertices;

	if (FAILED(hr = pDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &pVertexBuffer)))
	{
		fail(hr);
	}

	D3D11_BUFFER_DESC transformBufferDesc = {};
	transformBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	transformBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	transformBufferDesc.CPUAccessFlags = 0u;
	transformBufferDesc.MiscFlags = 0u;
	transformBufferDesc.ByteWidth = sizeof(TransformData);
	transformBufferDesc.StructureByteStride = 0u;

	if (FAILED(hr = pDevice->CreateBuffer(&transformBufferDesc, nullptr, &pTransformBuffer)))
	{
		fail(hr);
	}

	ReleaseCom(pVertexShaderBlob);
	ReleaseCom(pPixelShaderBlob);
}

Graphics::~Graphics()
{
	ReleaseCom(pTransformBuffer);
	ReleaseCom(pVertexBuffer);
	ReleaseCom(pInputLayout);
	ReleaseCom(pPixelShader);
	ReleaseCom(pVertexShader);
	ReleaseCom(pRenderTarget);
	ReleaseCom(pContext);
	ReleaseCom(pSwapChain);
	ReleaseCom(pDevice);
}

void Graphics::BeginFrame(float red, float green, float blue) noexcept
{
	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pRenderTarget, color);
}

void Graphics::DrawTestTriangle(float angle) noexcept
{
	const TransformData transform = { angle,{ 0.0f,0.0f,0.0f } };
	pContext->UpdateSubresource(pTransformBuffer, 0u, nullptr, &transform, 0u, 0u);

	const UINT stride = sizeof(Vertex);
	const UINT offset = 0u;
	pContext->IASetInputLayout(pInputLayout);
	pContext->IASetVertexBuffers(0u, 1u, &pVertexBuffer, &stride, &offset);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->VSSetShader(pVertexShader, nullptr, 0u);
	pContext->VSSetConstantBuffers(0u, 1u, &pTransformBuffer);
	pContext->PSSetShader(pPixelShader, nullptr, 0u);
	pContext->Draw(3u, 0u);
}

void Graphics::EndFrame()
{
	if (const HRESULT hr = pSwapChain->Present(1u, 0u); FAILED(hr))
	{
		throw BFGFX_EXCEPT(hr);
	}
}
