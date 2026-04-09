#pragma once
#include <DirectXMath.h>

class TopDownCamera
{
public:
	explicit TopDownCamera(float aspectRatio = 4.0f / 3.0f) noexcept;
	void SetAspectRatio(float aspectRatio) noexcept;
	void SetTarget(const DirectX::XMFLOAT3& target) noexcept;
	void RotateAroundTarget(float deltaRadians) noexcept;
	void AdjustZoom(float delta) noexcept;
	DirectX::XMFLOAT3 GetPosition() const noexcept;
	const DirectX::XMFLOAT3& GetTarget() const noexcept;
	DirectX::XMMATRIX GetViewMatrix() const noexcept;
	DirectX::XMMATRIX GetProjectionMatrix() const noexcept;
private:
	DirectX::XMFLOAT3 target = { 0.0f, 0.0f, 0.0f };
	float aspectRatio;
	float orbitAngle = DirectX::XM_PIDIV4;
	float radius = 12.0f;
	float height = 15.0f;
	float fov = DirectX::XMConvertToRadians(50.0f);
	float nearZ = 0.1f;
	float farZ = 150.0f;
};
