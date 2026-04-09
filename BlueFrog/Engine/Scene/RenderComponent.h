#pragma once
#include <DirectXMath.h>

enum class MeshType
{
	Plane,
	Cube,
};

struct RenderComponent
{
	MeshType mesh = MeshType::Cube;
	DirectX::XMFLOAT3 tint = { 1.0f, 1.0f, 1.0f };
};
