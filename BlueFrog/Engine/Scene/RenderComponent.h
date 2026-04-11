#pragma once
#include <DirectXMath.h>

enum class RenderMeshType
{
	Cube,
	Plane,
};

enum class RenderVisualKind
{
	SolidColor,
	Textured,
};

struct RenderComponent
{
	RenderMeshType meshType = RenderMeshType::Cube;
	DirectX::XMFLOAT3 tint = { 1.0f, 1.0f, 1.0f };
	RenderVisualKind visualKind = RenderVisualKind::SolidColor;
};
