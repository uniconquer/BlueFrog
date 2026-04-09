#include "TopDownCamera.h"
#include <algorithm>

TopDownCamera::TopDownCamera(float aspectRatio) noexcept
	:
	aspectRatio(aspectRatio)
{
}

void TopDownCamera::SetAspectRatio(float aspectRatio) noexcept
{
	this->aspectRatio = aspectRatio;
}

void TopDownCamera::SetTarget(const DirectX::XMFLOAT3& target) noexcept
{
	this->target = target;
}

void TopDownCamera::RotateAroundTarget(float deltaRadians) noexcept
{
	orbitAngle += deltaRadians;
}

void TopDownCamera::AdjustZoom(float delta) noexcept
{
	radius = std::clamp(radius + delta, 6.0f, 24.0f);
	height = std::clamp(height + delta * 0.75f, 8.0f, 30.0f);
}

DirectX::XMFLOAT3 TopDownCamera::GetPosition() const noexcept
{
	return
	{
		target.x - cosf(orbitAngle) * radius,
		target.y + height,
		target.z - sinf(orbitAngle) * radius
	};
}

const DirectX::XMFLOAT3& TopDownCamera::GetTarget() const noexcept
{
	return target;
}

DirectX::XMMATRIX TopDownCamera::GetViewMatrix() const noexcept
{
	using namespace DirectX;

	const auto position = GetPosition();
	return XMMatrixLookAtLH(
		XMVectorSet(position.x, position.y, position.z, 1.0f),
		XMVectorSet(target.x, target.y, target.z, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}

DirectX::XMMATRIX TopDownCamera::GetProjectionMatrix() const noexcept
{
	return DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, nearZ, farZ);
}
