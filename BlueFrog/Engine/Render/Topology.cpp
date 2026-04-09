#include "Topology.h"

Topology::Topology(D3D11_PRIMITIVE_TOPOLOGY type) noexcept
	:
	type(type)
{
}

void Topology::Bind(Graphics& gfx) const noexcept
{
	gfx.GetContext()->IASetPrimitiveTopology(type);
}
