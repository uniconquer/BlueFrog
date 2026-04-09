#pragma once
#include "../../Core/Graphics.h"

class Topology
{
public:
	explicit Topology(D3D11_PRIMITIVE_TOPOLOGY type) noexcept;
	void Bind(Graphics& gfx) const noexcept;
private:
	D3D11_PRIMITIVE_TOPOLOGY type;
};
