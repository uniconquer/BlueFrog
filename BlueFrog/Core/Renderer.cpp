#include "Renderer.h"
#include "Dxerr/dxerr.h"
#include <d3dcompiler.h>
#include <sstream>

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

Renderer::Exception::Exception(int line, const char* file, HRESULT hr) noexcept
	:
	BFException(line, file),
	hr(hr)
{
}

const char* Renderer::Exception::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode() << std::dec << std::endl
		<< "[Description] " << GetErrorString() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* Renderer::Exception::GetType() const noexcept
{
	return "BlueFrog Renderer Exception";
}

std::string Renderer::Exception::TranslateErrorCode(HRESULT hr) noexcept
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

HRESULT Renderer::Exception::GetErrorCode() const noexcept
{
	return hr;
}

std::string Renderer::Exception::GetErrorString() const noexcept
{
	return TranslateErrorCode(hr);
}

Renderer::Renderer(Graphics& gfx)
	:
	gfx(gfx)
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
		throw BFRENDER_EXCEPT(failedHr);
	};

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

	if (FAILED(hr = gfx.GetDevice()->CreateVertexShader(
		pVertexShaderBlob->GetBufferPointer(),
		pVertexShaderBlob->GetBufferSize(),
		nullptr,
		&pVertexShader)))
	{
		fail(hr);
	}

	if (FAILED(hr = gfx.GetDevice()->CreatePixelShader(
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

	if (FAILED(hr = gfx.GetDevice()->CreateInputLayout(
		inputElementDesc,
		static_cast<UINT>(sizeof(inputElementDesc) / sizeof(inputElementDesc[0])),
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

	if (FAILED(hr = gfx.GetDevice()->CreateBuffer(&vertexBufferDesc, &vertexData, &pVertexBuffer)))
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

	if (FAILED(hr = gfx.GetDevice()->CreateBuffer(&transformBufferDesc, nullptr, &pTransformBuffer)))
	{
		fail(hr);
	}

	ReleaseCom(pVertexShaderBlob);
	ReleaseCom(pPixelShaderBlob);
}

Renderer::~Renderer()
{
	ReleaseCom(pTransformBuffer);
	ReleaseCom(pVertexBuffer);
	ReleaseCom(pInputLayout);
	ReleaseCom(pPixelShader);
	ReleaseCom(pVertexShader);
}

void Renderer::DrawTestTriangle(float angle) noexcept
{
	const TransformData transform = { angle,{ 0.0f,0.0f,0.0f } };
	gfx.GetContext()->UpdateSubresource(pTransformBuffer, 0u, nullptr, &transform, 0u, 0u);

	const UINT stride = sizeof(Vertex);
	const UINT offset = 0u;
	gfx.GetContext()->IASetInputLayout(pInputLayout);
	gfx.GetContext()->IASetVertexBuffers(0u, 1u, &pVertexBuffer, &stride, &offset);
	gfx.GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gfx.GetContext()->VSSetShader(pVertexShader, nullptr, 0u);
	gfx.GetContext()->VSSetConstantBuffers(0u, 1u, &pTransformBuffer);
	gfx.GetContext()->PSSetShader(pPixelShader, nullptr, 0u);
	gfx.GetContext()->Draw(3u, 0u);
}
