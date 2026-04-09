#pragma once

enum class RenderMeshType
{
	Cube,
	Plane,
};

struct RenderComponent
{
	RenderMeshType meshType = RenderMeshType::Cube;
};
