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
	// Transient per-frame translational offset applied to BOTH the camera's
	// eye and look-at point inside GetViewMatrix. Caller pushes the offset
	// each frame and clears it (or pushes 0,0) after the render — the
	// camera does not persist it across ticks. Used by App to drive the
	// hit-impact screen shake without disturbing FollowPlayer state.
	void SetShakeOffsetXZ(float x, float z) noexcept;
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
	float shakeOffsetX = 0.0f;
	float shakeOffsetZ = 0.0f;
};
